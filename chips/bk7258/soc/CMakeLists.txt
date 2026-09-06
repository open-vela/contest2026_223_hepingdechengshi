if(CONFIG_ARCH_CHIP_BK7258)
	set(target bk7258_ap)
endif()

set(incs
	soc
	hal
	.
	)

list(APPEND incs
	${BEKEN_CHIPS_DIR}/app/ap/config/${target}/
	${BEKEN_CHIPS_DIR}/app/partitions/
)

set(srcs)
list(APPEND srcs
	"mpu_cfg.c"
	"hal/psram_ll.c"
	"hal/pwm_hal_debug.c"
	"hal/pwm_ll.c"
	"hal/timer_hal_debug.c"
	"hal/timer_ll.c"
	"hal/timer_hal.c"
	"hal/gpio_hal_debug.c"
	"hal/gpio_ll.c"
	"hal/dma_hal_debug.c"
	"hal/dma_ll.c"
	"hal/uart_hal_debug.c"
	"hal/uart_ll.c"
	"hal/icu_hal_debug.c"
	"hal/icu_ll.c"
	"hal/wdt_hal_debug.c"
	"hal/wdt_ll.c"
	"hal/trng_hal_debug.c"
	"hal/trng_ll.c"
	"hal/efuse_hal_debug.c"
	"hal/efuse_ll.c"
	"hal/adc_hal_debug.c"
	"hal/adc_ll.c"
	"hal/spi_hal_debug.c"
	"hal/spi_ll.c"
	"hal/sys_hal.c"
	"hal/sys_hal_debug.c"
	"hal/sys_pm_hal.c"
	"hal/sys_pm_hal_ctrl.c"
	"hal/sys_pm_hal_debug.c"
	"hal/i2c_hal_debug.c"
	"hal/i2c_ll.c"
	"hal/jpeg_hal_debug.c"
	"hal/jpeg_ll.c"
	"hal/calendar_hal_debug.c"
	"hal/calendar_ll.c"
	"hal/flash_hal_debug.c"
	"hal/flash_ll.c"
	"hal/aon_pmu_hal.c"
	"hal/aon_pmu_hal_debug.c"
	"hal/sdio_hal_debug.c"
	"hal/dma2d_hal_debug.c"
	"hal/yuv_buf_hal_debug.c"
	"hal/sys_clock_hal_debug.c"
	"hal/qspi_hal_debug.c"
	"hal/i2s_hal.c"
	"hal/sdmadc_hal_debug.c"
	"hal/ckmn_ll.c"
	"hal/ckmn_hal_debug.c"
	"hal/lin_ll.c"
	"hal/lin_hal_debug.c"
)

target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})

