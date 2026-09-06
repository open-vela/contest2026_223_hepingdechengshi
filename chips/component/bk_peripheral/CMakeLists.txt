set(BK_PERIPHERAL_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include(${BK_PERIPHERAL_PATH}/src/lcd/rgb/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/mcu/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/qspi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/lcd/spi/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/dvp/config.cmake)
include(${BK_PERIPHERAL_PATH}/src/tp/config.cmake)

set(incs include)
set(srcs
	src/bk_peripheral.c
)

if (CONFIG_LCD)
list(APPEND srcs
	src/lcd/lcd_panel_devices.c
)

list(APPEND srcs
	${RGB_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${MCU_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${QSPI_LCD_DEVICE_FILES}
)

list(APPEND srcs
	${SPI_LCD_DEVICE_FILES}
)
endif()

list(APPEND srcs
	${DVP_CAMERA_DEVICE_FILES}
)

list(APPEND srcs
	${TP_DEVICE_FILES}
)


set(priv_req bk_common)


target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})

