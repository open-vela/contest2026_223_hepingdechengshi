# Contest 2026 team 000 board placeholder
if(CONFIG_ARCH_BOARD_CONTEST2026_000_BOARD)
  add_subdirectory(src)
endif()
message("Nuttx Board:" ${NUTTX_BOARD_ABS_DIR})

set(LD_SCRIPT_FILE flash.ld)

set_property(GLOBAL PROPERTY LD_SCRIPT
                              "${NUTTX_BOARD_ABS_DIR}/scripts/${LD_SCRIPT_FILE}")