#pragma once

#include <nuttx/config.h>
#include <arch/irq.h>

/* BK7258 is an ARMv8-M Cortex-M33 SoC. */
#define ARMV8M_PERIPHERAL_INTERRUPTS (NR_IRQS - NVIC_IRQ_FIRST)
