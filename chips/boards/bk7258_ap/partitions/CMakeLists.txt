

if (CMAKE_BUILD_EARLY_EXPANSION)
return()
endif()

set(GEN_DIR ${CMAKE_CURRENT_BINARY_DIR}/_build)

set(GEN_FILE ${GEN_DIR}/security.h)
set(CONFIG_FILE_LISTS
	bl1_control.json
	../csv/ota.csv
	../csv/security.csv
	../csv/ppc.csv
	../csv/mpc.csv
	)


set(GEN_TOOL ${NUTTX_CHIP_ABS_DIR}/tools/build_tools/build_process/bk_sdk/bk_prebuild.py)

# bk_add_python_command injects the bk_sdk environment (PROJECT_DIR,
# PROJECT_BUILD_DIR, ARMINO_SOC_NAME, PROJECT_NAME, PYTHONPATH) and creates the
# output directory automatically. Defined in chips/cmake/bk_python.cmake.
bk_add_python_command(
	OUTPUT  ${GEN_FILE}
	SCRIPT  ${GEN_TOOL}
	ARGS    bk7258_ap
	DEPENDS ${CONFIG_FILE_LISTS}
	COMMENT "Generating security.h via bk_prebuild.py"
)

# DEPENDS (not SOURCES) is required to wire the custom-command rule into the
# build graph; SOURCES is IDE-only file listing and creates no build edge.
add_custom_target(generated_files DEPENDS ${GEN_FILE})
add_dependencies(arch generated_files)
target_include_directories(arch PRIVATE ${CMAKE_BINARY_DIR}/bk_build)

