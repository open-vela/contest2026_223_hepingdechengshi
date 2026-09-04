#include <nuttx/irq.h>
#include "cmsis_gcc.h"
#include "nvic.h"
#include "arm_internal.h"
#include "arch_interrupt.h"
void arm_ack_irq(int irq) {}
void up_irqinitialize(void) {
  /* Make sure the NVIC uses the NuttX vector table.  The per-CPU reset
   * handlers already point SCB->VTOR at _vectors, but assert it here too so
   * SVC (and the other exceptions) dispatch through exception_common.
   */
  putreg32((uint32_t)_vectors, NVIC_VECTAB);

  /* Attach the SVCall and Hard Fault exception handlers.  SVCall is the
   * context-switch mechanism (sys_call0 -> svc #0); HardFault must also be
   * caught because an SVCall can escalate to HardFault when raised from
   * within an ISR of equal or higher priority.
   */
  irq_attach(NVIC_IRQ_SVCALL, arm_svcall, NULL);
  irq_attach(NVIC_IRQ_HARDFAULT, arm_hardfault, NULL);

  /* Attach the other configurable fault handlers.  Without these a
   * MemManage/BusFault/UsageFault would land in the Default_Handler
   * infinite loop (no call stack to inspect), which is what the SHCSR
   * MEMFAULTENA bit enables.  arm_memfault/arm_busfault/arm_usagefault
   * are declared in arm_internal.h and compiled into libarch.
   */
  irq_attach(NVIC_IRQ_MEMFAULT, arm_memfault, NULL);
  irq_attach(NVIC_IRQ_BUSFAULT, arm_busfault, NULL);
  irq_attach(NVIC_IRQ_USAGEFAULT, arm_usagefault, NULL);

  /* The Beken boot path leaves both masks raised with raw CMSIS ops (NOT the
   * NuttX up_irq_disable(), which only raises BASEPRI): __start() does
   * `cpsid i` (PRIMASK=1), and soc_isr_init()/arch_isr_entry_init() ->
   * arch_int_init_all_irq() does `cpsid f` (FAULTMASK=1) + `cpsid i`.  If we
   * leave them set, the first context switch (SVC via sys_call0, and
   * PendSV/SysTick too) is masked and never taken: no exception ever reaches
   * arm_doirq and the scheduler hangs in thread mode.  Undo them the same
   * way: `cpsie f` clears FAULTMASK, `cpsie i` clears PRIMASK.
   */
  __enable_fault_irq();
  __enable_irq();
}
void up_enable_irq(int irq) {
  // enable interrupt with irq
  if (irq >= NVIC_IRQ_FIRST) {
    arch_int_enable_irq(irq);
  }
}
void up_disable_irq(int irq) {
  if (irq >= NVIC_IRQ_FIRST) {
    arch_int_disable_irq(irq);
  }
}
#ifdef CONFIG_ARCH_IRQPRIO
int up_prioritize_irq(int irq, int priority) {
  // set irq priority
}
#endif

// Obtain the current interrupt status
irqstate_t irqstate(void)
{
    return __get_xPSR();
}
