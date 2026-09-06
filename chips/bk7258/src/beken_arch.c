#include <stdint.h>

#include <assert.h>

#include "smp.h"
#include "sys_driver.h"
#include <driver/mailbox.h>
#include <nuttx/arch.h>
#include <nuttx/config.h>
#include <nuttx/sched.h>
#include <syslog.h>
#include "cmsis_gcc.h"
#include "easy_log.h"
#include "beken_arch.h"
/* nxsched_process_delivered is declared in the internal sched/sched.h;
 * forward-declare it here to avoid pulling in private kernel headers. */
void nxsched_process_delivered(int cpu);

/* nx_idle_trampoline is the NuttX secondary CPU entry point.
 * Forward-declare to avoid pulling in init/init.h. */
void nx_idle_trampoline(void);

/* bk_mailbox_master_send is defined in mbox0_cross_core.c but has no
 * public header declaration — forward-declare it here. */
bk_err_t bk_mailbox_master_send(mailbox_data_t *data, uint8_t src, uint8_t dst);

/* reset_cpu1_core is defined in bk_startup/system_main.c — releases
 * physical AP CPU1 from reset after programming its boot vector address. */
void reset_cpu1_core(uint32_t offset, uint32_t start_flag);

/* Beken per-CPU startup: multicore_cpu1_func is set by the primary core
 * before releasing CPU1 from reset.  CPU1's Reset_Handler_Cpu1 calls
 * _othercore_start() which invokes this function pointer to enter the OS.
 * Defined in startup_cpu1.c. */
extern void (*multicore_cpu1_func)(void);

/* CPU1 vector table base address, defined by the linker script
 * (bk7258_ap_bsp.ld) at a 512-byte aligned offset in the .vectors section. */
extern uint32_t __vector_core1_table;

extern uint32_t __StackTopCpu0;
extern uint32_t __StackLimitCpu0;

extern uint32_t __StackTopCpu1;
extern uint32_t __StackLimitCpu1;

#define NUTTX_STACK_SIZE (4 << 10);

/* Idle stack pool exported by the linker script.
 * Total size: CONFIG_SMP_NCPUS * CONFIG_IDLETHREAD_STACKSIZE.
 * CPU n's stack_alloc = g_cpu_stackalloc + CONFIG_IDLETHREAD_STACKSIZE * (n +
 * 1). */
extern uint8_t g_cpu_stackalloc[];

int up_cpu_index(void) { return nuttx_cpu_get_core_id(); }

/****************************************************************************
 * Name: up_cpu_start
 *
 * Description:
 *   In an SMP configuration, the primary CPU (CPU0) calls this function to
 *   start a secondary CPU.  Only CPU 1 is started here (CPU 0 is already
 *   active).
 *
 *   The secondary CPU (physical CPU1) boots through its own reset handler
 *   (Reset_Handler_Cpu1), which calls _othercore_start().  That function
 *   reads the global function pointer 'multicore_cpu1_func' and invokes
 *   it to enter the OS.  We set that pointer to NuttX's nx_idle_trampoline
 *   and then release CPU1 from reset via the Beken HAL.
 *
 * Input Parameters:
 *   cpu - The index of the CPU being started (1 for the secondary core).
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/
static volatile bool g_trace_core2_debug = true;
static void multicore_start_debug_func(void) {
  __enable_fault_irq();
	__enable_irq();
  EA_LOG_D("multicore start function");
  
  nx_idle_trampoline();
}
extern void reset_cpu2_core(uint32 offset, uint32_t start_flag);
int up_cpu_start(int cpu)
{
  /* Only CPU 1 can be started (CPU 0 is already running). */
  if (cpu != 1)
    {
      return -EINVAL;
    }

  /* Set the entry point that Reset_Handler_Cpu1 → _othercore_start()
   * will call once AP CPU1 comes out of reset.
   */
  multicore_cpu1_func = multicore_start_debug_func;

  /* Boot AP CPU1: reset_cpu1_core() programs the boot vector address
   * (__vector_core1_table) and releases the reset, causing CPU1 to
   * execute Reset_Handler_Cpu1 → _othercore_start() → our callback.
   *
   * Defined in vendor/beken/chips/component/bk_startup/system_main.c.
   */
  reset_cpu2_core((uint32_t)&__vector_core1_table, 1);

  return 0;
}
/* -------------------------------------------------------------------------
 * SMP inter-core notification via BK7258 cross-core mailbox (mbox0).
 *
 * This uses bk_mailbox_master_send / crosscore_mb_rx_isr -- the same
 * low-level mbox0 path used by the FreeRTOS SMP port -- which is
 * completely separate from the higher-level bk_mailbox_send channel
 * layer.  No conflict with mailbox_channel.c's registrations.
 *
 * up_cpu_index() returns a 0-based logical index (0 or 1).  The mbox0
 * driver uses physical CPU IDs, so all conversions use:
 *   physical_id = logical_cpu + MAILBOX_CPU1   (AP = physical CPU1/CPU2)
 *
 * bk_mailbox_cc_init_on_current_core() is already called from
 * startup_cpu1.c / startup_cpu2.c before the scheduler starts; it
 * registers crosscore_smp_cmd_handler as the mbox0 receive callback,
 * which in turn calls crosscore_mb_rx_isr() below.
 * No separate init step is needed in beken_arch.c.
 * ------------------------------------------------------------------------- */

/* Convert a NuttX logical CPU index (0-based) to a physical CPU ID. */
#define LOGICAL_TO_PHYS(n) ((uint8_t)((n) + MAILBOX_CPU1))

/* Command identifiers (stored in mailbox_data_t.param2, matching the
 * layout expected by bk_mailbox_master_send / crosscore_smp_cmd_handler). */

/**
 * crosscore_mb_rx_isr - mbox0 receive handler for NuttX SMP.
 *
 * Called by crosscore_smp_cmd_handler (mbox0_cross_core.c) whenever
 * another core sends a message.  Replaces the FreeRTOS version.
 *
 * param0 = physical source CPU ID
 * param2 = command (BK_NUTTX_SMP_CMD_SCHED or BK_NUTTX_SMP_CMD_CALL)
 */
void crosscore_mb_rx_isr(mailbox_data_t *data) {
  uint32_t cmd = data->param2;
  syslog(LOG_INFO, "crosscore_mb_rx_isr[%d]:%d \r\n", up_cpu_index(), (int)cmd);
  if (cmd == BK_NUTTX_SMP_CMD_SCHED || cmd == BK_NUTTX_SMP_CMD_CALL) {
    /* Process pending cross-core function calls, then deliver tasks.
     * If either queue is empty, the handler is a cheap no-op. */
    nxsched_smp_call_handler(0, NULL, NULL);
    nxsched_process_delivered(up_cpu_index());
  }
}

int up_send_smp_sched(int cpu) {
  int from = up_cpu_index();
  mailbox_data_t data;

  data.param0 = (uint32_t)LOGICAL_TO_PHYS(from); /* source physical ID */
  data.param1 = (uint32_t)LOGICAL_TO_PHYS(cpu);  /* dest physical ID   */
  data.param2 = BK_NUTTX_SMP_CMD_SCHED;
  data.param3 = 0;

  bk_mailbox_master_send(&data, LOGICAL_TO_PHYS(from), LOGICAL_TO_PHYS(cpu));
  return OK;
}

void up_send_smp_call(cpu_set_t cpuset) {
  int from = up_cpu_index();
  int cpu;
  mailbox_data_t data;

  /* Send one mbox0 message per target CPU; skip self (already handled). */
  while (cpuset != 0) {
    cpu = ffs((int)cpuset) - 1;
    cpuset &= ~((cpu_set_t)1 << cpu);

    if (cpu == from) {
      continue;
    }

    data.param0 = (uint32_t)LOGICAL_TO_PHYS(from);
    data.param1 = (uint32_t)LOGICAL_TO_PHYS(cpu);
    data.param2 = BK_NUTTX_SMP_CMD_CALL;
    data.param3 = 0;

    bk_mailbox_master_send(&data, LOGICAL_TO_PHYS(from), LOGICAL_TO_PHYS(cpu));
  }
}
int bk_cross_core_send(uint32_t cmd) {
  syslog(LOG_INFO, "bk_cross_core_send %d", cmd);
  int from = up_cpu_index();
  mailbox_data_t data;

  data.param0 = (uint32_t)LOGICAL_TO_PHYS(from); /* source physical ID */
  data.param1 = (uint32_t)LOGICAL_TO_PHYS(1 - from);  /* dest physical ID   */
  data.param2 = cmd;
  data.param3 = 0;

  bk_mailbox_master_send(&data, LOGICAL_TO_PHYS(from), LOGICAL_TO_PHYS(1 - from));
  return OK;
}
uintptr_t up_get_intstackbase(int cpu) {
  if (cpu == 0) {
    return __StackLimitCpu0;
  } else if (cpu == 1) {
    return __StackLimitCpu1;
  }
  return 0;
}
int up_cpu_idlestack(int cpu, struct tcb_s *tcb, size_t stack_size) {
  uintptr_t stack_alloc;

  DEBUGASSERT(cpu >= 0 && cpu < CONFIG_NCPUS && tcb != NULL);

  /* Each CPU gets a dedicated slot in the g_cpu_stackalloc pool.
   * CPU n occupies [n*SIZE, (n+1)*SIZE), so stack_alloc points to
   * the top of that slot: g_cpu_stackalloc + (n + 1) * SIZE. */

  stack_alloc =
      (uintptr_t)g_cpu_stackalloc + CONFIG_IDLETHREAD_STACKSIZE * (cpu);

  DEBUGASSERT((stack_alloc & STACKFRAME_ALIGN_MASK) == 0);

  tcb->adj_stack_size = stack_size;
  tcb->stack_alloc_ptr = (void *)stack_alloc;
  tcb->stack_base_ptr = tcb->stack_alloc_ptr;

  return OK;
}
