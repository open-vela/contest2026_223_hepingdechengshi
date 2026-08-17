# Contest 2026 team 000 board placeholder
if(CONFIG_ARCH_BOARD_CONTEST2026_000_BOARD)
  add_subdirectory(src)

  
  # Expose the generated header to the board sources (and any target that
  # inherits board's interface include dirs).
  target_include_directories(board PUBLIC ${CMAKE_BINARY_DIR}/include)

  # Firmware packaging is handled by scripts/post_build.cmake.
  # nuttx/CMakeLists.txt includes that file after add_subdirectory(boards),
  # still in the nuttx/ scope where the 'nuttx' target was created, so
  # add_custom_command(TARGET nuttx POST_BUILD) is valid there.
endif()
message("Nuttx Board:" ${NUTTX_BOARD_ABS_DIR} ${CONFIG_ARCH_BOARD_CONTEST2026_000_BOARD})

set(LD_SCRIPT_FILE bk7258_ap_bsp.ld)

set_property(GLOBAL PROPERTY LD_SCRIPT
                              "${NUTTX_BOARD_ABS_DIR}/scripts/${LD_SCRIPT_FILE}")

