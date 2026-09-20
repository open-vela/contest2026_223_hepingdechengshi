#ifndef BEKEN_IRQ_H
#define BEKEN_IRQ_H
#include <arch/irq.h>
#define IRQ_NORMAL(x) (x + NVIC_IRQ_FIRST)
void common_irq_initialize_on_core(void);
#endif
