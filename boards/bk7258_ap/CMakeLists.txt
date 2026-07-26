# Contest 2026 team 000 board placeholder
if(CONFIG_ARCH_BOARD_CONTEST2026_000_BOARD)
  add_subdirectory(src)

  # Generate ram_regions.h from ram_regions.csv. The wrapper script drives the
  # reusable bk_ram_region library with explicit paths (no PROJECT_* env).
  set(RAM_REGIONS_CSV ${NUTTX_BOARD_ABS_DIR}/scripts/ram_regions.csv)
  set(RAM_REGIONS_GEN ${NUTTX_BOARD_ABS_DIR}/scripts/gen_ram_regions.py)
  set(RAM_REGIONS_HDR ${CMAKE_BINARY_DIR}/include/ram_regions.h)

  # Generate at CONFIGURE time. The pre-compiled linker script
  # (${LD_SCRIPT_FILE}) #include's ram_regions.h, and its ldscript_tmp target
  # (nuttx/CMakeLists.txt) does not depend on this header, so with -j it can be
  # preprocessed before a build-time generator runs. Producing the header now
  # guarantees it exists on disk before any build step starts.
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${RAM_REGIONS_GEN} --csv ${RAM_REGIONS_CSV}
            --out ${RAM_REGIONS_HDR}
    RESULT_VARIABLE RAM_REGIONS_RESULT)
  if(NOT RAM_REGIONS_RESULT EQUAL 0)
    message(FATAL_ERROR "bk7258_ap: failed to generate ram_regions.h")
  endif()

  # Re-run CMake configure when the table or generator changes, so the
  # configure-time header above stays in sync.
  set_property(
    DIRECTORY APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${RAM_REGIONS_CSV} ${RAM_REGIONS_GEN})

  # Also expose it as a build-time target: regenerates the header if the table
  # changes within a build, and gives `board` an explicit dependency on it.
  add_custom_command(
    OUTPUT ${RAM_REGIONS_HDR}
    COMMAND ${Python3_EXECUTABLE} ${RAM_REGIONS_GEN} --csv ${RAM_REGIONS_CSV}
            --out ${RAM_REGIONS_HDR}
    DEPENDS ${RAM_REGIONS_CSV} ${RAM_REGIONS_GEN}
    COMMENT "bk7258_ap: regenerating ram_regions.h")

  add_custom_target(bk7258_ap_ram_regions DEPENDS ${RAM_REGIONS_HDR})

  # Generate partitions_gen.h from auto_partitions.csv. Same pattern as
  # ram_regions above: the wrapper drives the reusable bk_auto_partition /
  # bk_flash_partiton libraries with explicit paths (no PROJECT_* env). The
  # linker script pulls in "partitions/partitions.h" (found via the chip dir on
  # the LD preprocess include path), which in turn #include's "partitions_gen.h"
  # — resolved here from ${CMAKE_BINARY_DIR}/include.
  set(PARTITIONS_CSV ${NUTTX_BOARD_ABS_DIR}/scripts/auto_partitions.csv)
  set(PARTITIONS_GEN ${NUTTX_BOARD_ABS_DIR}/scripts/gen_partitions.py)
  set(PARTITIONS_HDR ${CMAKE_BINARY_DIR}/include/partitions_gen.h)

  # Generate at CONFIGURE time so the header exists on disk before the parallel
  # ldscript_tmp preprocess step (which does not depend on it) can run.
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${PARTITIONS_GEN} --csv ${PARTITIONS_CSV}
            --out ${PARTITIONS_HDR}
    RESULT_VARIABLE PARTITIONS_RESULT)
  if(NOT PARTITIONS_RESULT EQUAL 0)
    message(FATAL_ERROR "bk7258_ap: failed to generate partitions_gen.h")
  endif()

  # Re-run CMake configure when the table or generator changes.
  set_property(
    DIRECTORY APPEND
    PROPERTY CMAKE_CONFIGURE_DEPENDS ${PARTITIONS_CSV} ${PARTITIONS_GEN})

  # Build-time target: regenerates the header if the table changes within a
  # build, and gives `board` an explicit dependency on it.
  add_custom_command(
    OUTPUT ${PARTITIONS_HDR}
    COMMAND ${Python3_EXECUTABLE} ${PARTITIONS_GEN} --csv ${PARTITIONS_CSV}
            --out ${PARTITIONS_HDR}
    DEPENDS ${PARTITIONS_CSV} ${PARTITIONS_GEN}
    COMMENT "bk7258_ap: regenerating partitions_gen.h")

  add_custom_target(bk7258_ap_partitions DEPENDS ${PARTITIONS_HDR})

  # Make the NuttX `board` kernel library depend on the generated headers.
  # `board` is created in nuttx/boards/CMakeLists.txt before this subdirectory
  # is added, so it is guaranteed to exist here.
  add_dependencies(board bk7258_ap_ram_regions bk7258_ap_partitions)

  # Expose the generated header to the board sources (and any target that
  # inherits board's interface include dirs).
  target_include_directories(board PUBLIC ${CMAKE_BINARY_DIR}/include)
endif()
message("Nuttx Board:" ${NUTTX_BOARD_ABS_DIR} ${CONFIG_ARCH_BOARD_CONTEST2026_000_BOARD})

set(LD_SCRIPT_FILE bk7258_ap_bsp.ld)

set_property(GLOBAL PROPERTY LD_SCRIPT
                              "${NUTTX_BOARD_ABS_DIR}/scripts/${LD_SCRIPT_FILE}")

