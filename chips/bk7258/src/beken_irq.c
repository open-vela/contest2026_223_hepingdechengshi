#include "arch_interrupt.h"
#include "arm_internal.h"
#include "cmsis_gcc.h"
#include "nvic.h"
#include <nuttx/irq.h>

/* Given the address of a NVIC ENABLE register, this is the offset to
 * the corresponding CLEAR ENABLE register.
 */
#define NVIC_ENA_OFFSET (0)
#define NVIC_CLRENA_OFFSET (NVIC_IRQ0_31_CLEAR - NVIC_IRQ0_31_ENABLE)

/****************************************************************************
 * Name: beken_irqinfo
 *
 * Description:
 *   Given an IRQ number, provide the register and bit setting to enable or
 *   disable the irq.
 *
 ****************************************************************************/
/* Get a 32-bit version of the default priority */
#define DEFPRIORITY32                                                          \
  (NVIC_SYSH_PRIORITY_DEFAULT << 24 | NVIC_SYSH_PRIORITY_DEFAULT << 16 |       \
   NVIC_SYSH_PRIORITY_DEFAULT << 8 | NVIC_SYSH_PRIORITY_DEFAULT)

static int beken_irqinfo(int irq, uintptr_t *regaddr, uint32_t *bit,
                         uintptr_t offset) {
  int n;

  /* Check for external interrupt */

  if (irq >= NVIC_IRQ_FIRST) {
    n = irq - NVIC_IRQ_FIRST;
    *regaddr = NVIC_IRQ_ENABLE(n) + offset;
    *bit = (uint32_t)1 << (n & 0x1f);
  }

  /* Handle processor exceptions.  Only a few can be disabled */

  else {
    *regaddr = NVIC_SYSHCON;
    if (irq == NVIC_IRQ_MEMFAULT)
      *bit = NVIC_SYSHCON_MEMFAULTENA;
    else if (irq == NVIC_IRQ_BUSFAULT)
      *bit = NVIC_SYSHCON_BUSFAULTENA;
    else if (irq == NVIC_IRQ_USAGEFAULT)
      *bit = NVIC_SYSHCON_USGFAULTENA;
    else if (irq == NVIC_IRQ_SYSTICK) {
      *regaddr = NVIC_SYSTICK_CTRL;
      *bit = NVIC_SYSTICK_CTRL_ENABLE;
    } else
      return ERROR; /* Invalid or unsupported exception */
  }

  return OK;
}
static inline void beken_prioritize_syscall(int priority)
{
  uint32_t regval;

  /* SVCALL is system handler 11 */

  regval  = getreg32(NVIC_SYSH8_11_PRIORITY);
  regval &= ~NVIC_SYSH_PRIORITY_PR11_MASK;
  regval |= (priority << NVIC_SYSH_PRIORITY_PR11_SHIFT);
  putreg32(regval, NVIC_SYSH8_11_PRIORITY);
}
void arm_ack_irq(int irq) {}
void common_irq_initialize_on_core(void) {
    uint32_t regaddr;
  int num_priority_registers;
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
  putreg32(DEFPRIORITY32, NVIC_SYSH4_7_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH8_11_PRIORITY);
  putreg32(DEFPRIORITY32, NVIC_SYSH12_15_PRIORITY);
  beken_prioritize_syscall(NVIC_SYSH_SVCALL_PRIORITY);

  num_priority_registers = (getreg32(NVIC_ICTR) + 1) * 8;

  /* Now set all of the interrupt lines to the default priority */

  regaddr = NVIC_IRQ0_3_PRIORITY;
  while (num_priority_registers--)
    {
      putreg32(DEFPRIORITY32, regaddr);
      regaddr += 4;
    }
}
void up_irqinitialize(void) {
  common_irq_initialize_on_core();
  __enable_fault_irq();
  __enable_irq();
}
void up_enable_irq(int irq) {
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t bit;

  if (beken_irqinfo(irq, &regaddr, &bit, NVIC_ENA_OFFSET) == OK) {
    /* Modify the appropriate bit in the register to enable the interrupt.
     * For normal interrupts, we need to set the bit in the associated
     * Interrupt Set Enable register.  For other exceptions, we need to
     * set the bit in the System Handler Control and State Register.
     */

    if (irq >= NVIC_IRQ_FIRST) {
      arch_int_enable_irq(irq);
    } else {
      regval = getreg32(regaddr);
      regval |= bit;
      putreg32(regval, regaddr);
    }
  }
}

void up_disable_irq(int irq) {
  uintptr_t regaddr;
  uint32_t regval;
  uint32_t bit;

  if (beken_irqinfo(irq, &regaddr, &bit, NVIC_CLRENA_OFFSET) == OK) {
    /* Modify the appropriate bit in the register to disable the interrupt.
     * For normal interrupts, we need to set the bit in the associated
     * Interrupt Clear Enable register.  For other exceptions, we need to
     * clear the bit in the System Handler Control and State Register.
     */

    if (irq >= NVIC_IRQ_FIRST) {
      arch_int_disable_irq(irq);
    } else {
      regval = getreg32(regaddr);
      regval &= ~bit;
      putreg32(regval, regaddr);
    }
  }
}
#ifdef CONFIG_ARCH_IRQPRIO
int up_prioritize_irq(int irq, int priority) {
  // set irq priority
}
#endif

// Obtain the current interrupt status
irqstate_t irqstate(void) { return __get_xPSR(); }
