/****************************************************************************
 * Contest 2026 team 000 board - boot stub (no-op placeholder)
 ****************************************************************************/

#include <nuttx/board.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <syslog.h>

#include "beken_arch.h"
#include "beken_uart.h"
#include "bk_private/components_init.h"
#include "bk_rtos_debug.h"
#include "board.h"
#include "driver/lcd.h"
#include "driver/psram.h"
#include "driver/pwr_clk.h"
#include "lcd_panel_devices.h"
/****************************************************************************
 * LCD ST7701SN Initialization
 ****************************************************************************/

/* Board LCD initialization - uses pre-defined lcd_device_st7701sn */

int board_lcd_initialize(void) {
  bk_err_t ret;

  syslog(LOG_INFO, "=== Board LCD ST7701SN RGB888 Init ===\n");

  /* Use the pre-defined device from bk_peripheral */
  ret = lcd_driver_init(&lcd_device_st7701sn);
  if (ret != BK_OK) {
    syslog(LOG_ERR, "LCD driver init failed: %d\n", ret);
    return 0;
  }

  ret = lcd_driver_backlight_open();
  if (ret != BK_OK) {
    syslog(LOG_ERR, "LCD backlight open failed: %d\n", ret);
    return 0;
  }

  lcd_driver_backlight_set(100);

  ret = lcd_driver_display_enable();
  if (ret != BK_OK) {
    syslog(LOG_ERR, "LCD display enable failed: %d\n", ret);
    return 0;
  }

  syslog(LOG_INFO, "LCD ST7701SN initialized: 480x480 RGB888\n");
}

void openvela_board_initialize(void) {
  /* Placeholder: no hardware initialization. */
}

int board_app_initialize(uintptr_t arg) {
  (void)arg;
  return 0;
}

/****************************************************************************
 * SMP Test Tasks
 ****************************************************************************/

static int test_task_entry(int argc, char *argv[]) {
  int task_id = atoi(argv[1]);
  bk_cross_core_send(BK_NUTTX_SMP_CMD_TEST);
  while (1) {
    int cpu_id = up_cpu_index();
    printf("Task %d Run on CPU %d\n", task_id, cpu_id);
    usleep(1000000); /* 1 second */
  }

  return 0;
}

static void create_smp_test_tasks(void) {
  char *argv1[] = {"test_task", "1", NULL};
  char *argv2[] = {"test_task", "2", NULL};

  /* Create task 1 */
  int pid1 = task_create("test_task1", 100, 2048, test_task_entry, argv1);
  if (pid1 < 0) {
    syslog(LOG_ERR, "Failed to create test_task1\n");
  } else {
    syslog(LOG_INFO, "Created test_task1, PID=%d\n", pid1);
  }

  /* Create task 2 */
  int pid2 = task_create("test_task2", 100, 2048, test_task_entry, argv2);
  if (pid2 < 0) {
    syslog(LOG_ERR, "Failed to create test_task2\n");
  } else {
    syslog(LOG_INFO, "Created test_task2, PID=%d\n", pid2);
  }
}

static void beken_late_bringup(void) {
  set_ap_startup_index(AP_NX_LATE_INIT_START);
  components_init();
  bk_pm_cp1_boot_ok_response_set();
#if (CONFIG_PSRAM)
  bk_psram_id_auto_detect();
#endif
  set_ap_startup_index(AP_NX_LATE_INIT_DONE);
  beken_uart_init();
  syslog(LOG_INFO, "Hello Easy Log");

  /* Create SMP test tasks */
  create_smp_test_tasks();
}

#ifdef CONFIG_BOARD_LATE_INITIALIZE
void board_late_initialize(void) {
  /* Perform board-specific initialization. */
  components_early_init_stage2();
  bk_module_init();
  beken_late_bringup();

  /* Initialize LCD ST7701SN RGB888 */
}
#endif /* CONFIG_BOARD_LATE_INITIALIZE */
