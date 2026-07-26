#include <nuttx/irq.h>

void arm_ack_irq(int irq) {}
void up_irqinitialize(void) {
  // Disable all interrupts
  // Set the NVIC vector location
  // Set all interrupts (and exceptions) to the default priority
  // Attach the SVCall and Hard Fault exception handlers
  // enable interrupts
}
void up_enable_irq(int irq) {
  // enable interrupt with irq
}
void up_disable_irq(int irq) {
  // disable interrupt with irq
}
#ifdef CONFIG_ARCH_IRQPRIO
int up_prioritize_irq(int irq, int priority) {
  // set irq priority
}
#endif

// Obtain the current interrupt status
irqstate_t irqstate(void)
{
    return 0;
}