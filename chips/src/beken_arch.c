#include <stdint.h>

#include <assert.h>

#include "smp.h"
#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/sched.h>

extern uint32_t __StackTopCpu0;
extern uint32_t __StackLimitCpu0;

extern uint32_t __StackTopCpu1;
extern uint32_t __StackLimitCpu1;

#define NUTTX_STACK_SIZE (4 << 10);

/* Idle stack pool exported by the linker script.
 * Total size: CONFIG_SMP_NCPUS * CONFIG_IDLETHREAD_STACKSIZE.
 * CPU n's stack_alloc = g_cpu_stackalloc + CONFIG_IDLETHREAD_STACKSIZE * (n + 1). */
extern uint8_t g_cpu_stackalloc[];

int up_cpu_index(void) { return cpu_get_core_id(); }
int up_cpu_start(int cpu) {
  (void)cpu;
  return 0;
}
int up_send_smp_sched(int cpu) {
  (void)cpu;
  return 0;
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

  stack_alloc = (uintptr_t)g_cpu_stackalloc +
                CONFIG_IDLETHREAD_STACKSIZE * (cpu + 1);

  DEBUGASSERT((stack_alloc & STACKFRAME_ALIGN_MASK) == 0);

  tcb->adj_stack_size = stack_size;
  tcb->stack_alloc_ptr = (void *)stack_alloc;
  tcb->stack_base_ptr = tcb->stack_alloc_ptr;

  return OK;
}
void up_send_smp_call(cpu_set_t cpuset) { (void)cpuset; };
