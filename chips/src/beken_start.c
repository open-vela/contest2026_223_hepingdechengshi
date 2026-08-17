/****************************************************************************
 * 
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdarg.h>
#include <stdio.h>

#include <nuttx/arch.h>
#include <nuttx/init.h>
#include <nuttx/cache.h>

#include <arch/board/board.h>
#include "arm_internal.h"
#include "beken_uart.h"
#include "os/os.h"
#include "bk_rtos_debug.h"
#include "armstar.h"
/* Force the Beken per-CPU vector table objects to be linked into the
 * final ELF.  They live in libarch.a and would otherwise be discarded
 * because no symbol from startup_cpu*.o is directly referenced.
 */
extern void Reset_Handler_Cpu0(void);
static void (*__bk_ref_cpu0_reset)(void)
    __attribute__((used)) = &Reset_Handler_Cpu0;

#define showprogress(c) arm_lowputc(c)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* .data is positioned first in the primary RAM followed immediately by .bss.
 * The IDLE thread stack lies just after .bss and has size give by
 * CONFIG_IDLETHREAD_STACKSIZE;  The heap then begins just after the IDLE.
 * ARM EABI requires 64 bit stack alignment.
 */




void __start(void)
{
  uint32_t *dest;
  const uint32_t *src;
  set_ap_startup_index(AP_NX_START_ENTER);
  /* Disable all interrupts at the very beginning to prevent any ISR
   * from firing during initialization. This is critical because HAL_Init()
   * and other early initialization code may trigger hardware interrupts
   * before NuttX interrupt system is ready.
   */
  __asm volatile ("cpsid i" : : : "memory");
  SCB->VTOR = (uint32_t)_vectors;
  /* SCB_VTOR is set by each CPU's reset handler before entering __start().
   * CPU0: Reset_Handler_Cpu0 → SCB->VTOR = &__vector_core0_table
   * CPU1: Reset_Handler_Cpu1 → enters via callback, not __start()
   */

  /* Configure FPU before any floating point operations */

  arm_fpuconfig();

  /* Clear BSS section - critical for proper variable initialization */

  // for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
  //   {
  //     *dest++ = 0;
  //   }

  /* Copy initialized data from flash to SRAM */

  // for (src = (const uint32_t *)_eronly,
  //      dest = (uint32_t *)_sdata; dest < (uint32_t *)_edata; )
  //   {
  //     *dest++ = *src++;
  //   }

  // /* Copy .ramfunc section from flash to SRAM */

  // for (src = (const uint32_t *)&_siramfunc,
  //      dest = (uint32_t *)&_sramfunc; dest < (uint32_t *)&_eramfunc; )
  //   {
  //     *dest++ = *src++;
  //   }

  arm_lowputc('A'); /* data segment init done */

#ifdef CONFIG_ARMV8M_ICACHE
  up_enable_icache();
#endif

#ifdef CONFIG_ARMV8M_DCACHE
  up_enable_dcache();
#endif
    arm_lowputc('B'); /* cache enable done */

  /* Call HAL_Init() with interrupts disabled.
   * Some HAL functions may trigger hardware events that could
   * generate interrupts, but they won't fire while interrupts are disabled.
   */
  // HAL_Init();
  arm_lowputc('C'); /* HAL init done */

  /* Disable SysTick that was enabled by HAL_Init().
   * NuttX uses its own timer system (LPTIM for tickless mode).
   * SysTick must be disabled to prevent unexpected interrupts.
   */
#define NVIC_SYSTICK_CTRL_REG   (*((volatile uint32_t *)0xE000E010))
  NVIC_SYSTICK_CTRL_REG = 0;  /* Disable SysTick completely */


  arm_lowputc('D'); /* about to start system */

  /* nx_start() will initialize the interrupt system and enable interrupts.
   * Interrupts remain disabled until the system is fully ready.
   */
  rtos_start_scheduler();

  showprogress('X'); /* should never reach here */

  for (; ; );
}

#define HEAP_BASE      ((uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE)
const uintptr_t g_idle_topstack = HEAP_BASE;

