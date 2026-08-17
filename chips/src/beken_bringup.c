#include "armstar.h"
#include "beken_uart.h"
#include "bk_pm_internal_api.h"
#include "bk_private/bk_driver.h"
#include "bk_private/components_init.h"
#include "bk_rtos_debug.h"
#include "core_star.h"
#include "driver/mailbox_channel.h"
#include "driver/psram.h"
#include "driver/pwr_clk.h"
#include "easy_log.h"
#include "mpu_api.h"
#include <nuttx/arch.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <debug.h>

/* Heap boundary symbols exported by the linker script
 * (boards/bk7258_ap/scripts/bk7258_ap_bsp.ld).  The .heap section spans
 * [_heap_start, _heap_end): it begins just after the .cpu_idle_stacks pool
 * and extends to just before the per-CPU MSP stacks reserved at the top of
 * RAM.
 */

extern uint8_t _heap_start[];
extern uint8_t _heap_end[];

/****************************************************************************
 * Name: up_allocate_heap
 *
 * Description:
 *   Return the user heap region to NuttX for the flat build.  Overrides the
 *   weak default in arch/arm/src/common/arm_allocateheap.c, which relies on
 *   CONFIG_RAM_START/CONFIG_RAM_SIZE/CONFIG_RAM_END.  None of those are
 *   defined for BK7258 (CONFIG_RAM_END falls back to 0), so the heap bounds
 *   are taken directly from the linker script symbols instead.
 *
 ****************************************************************************/

void up_allocate_heap(void **heap_start, size_t *heap_size) {
  *heap_start = (void *)_heap_start;
  *heap_size = (size_t)(_heap_end - _heap_start);
}

__attribute__((weak)) void bk_module_init(void) {}
extern void __start(void);
void _entry_main(void) {
  set_ap_startup_index(AP_ENTER_ENTRY_MAIN);
  if (components_early_init_stage1())
    return;
  set_ap_startup_index(AP_NX_START_PREPARE_DONE);
  __start();
  return;
}
volatile bool loop_here = true;
void beken_bringup(void) {

#if CONFIG_MPU
  mpu_enable();
#endif // #if CONFIG_MPU

#if CONFIG_DCACHE
  if (SCB->CLIDR & SCB_CLIDR_DC_Msk)
    SCB_EnableDCache();

  SCB_CleanInvalidateDCache();
#endif

#if CONFIG_CM_BACKTRACE
  cm_backtrace_init(FIREWARE_NAME, HARDWARE_VERSION, SOFTWARE_VERSION);
#endif

  /*power manager init*/
  pm_hardware_init();

  // while(loop_here);
  _entry_main();
}
void beken_late_bringup(void) {
  set_ap_startup_index(AP_NX_LATE_INIT_START);
  components_init();
  bk_pm_cp1_boot_ok_response_set();
#if (CONFIG_PSRAM)
  bk_psram_id_auto_detect();
#endif
  set_ap_startup_index(AP_NX_LATE_INIT_DONE);
  beken_uart_init();
  syslog(LOG_INFO, "Hello Easy Log");
}
#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void) {
  /* Perform board-specific initialization */
  components_early_init_stage2();
  bk_module_init();
  beken_late_bringup();
}
#endif
// void up_initialize(void) {
//   components_early_init_stage2();
//   bk_module_init();
// }

void arm_serialinit(void) {
  uart_register("/dev/console", GET_CONSOLE_SERIAL());

}
