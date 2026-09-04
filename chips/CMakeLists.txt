# ##############################################################################
# 
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################


message("CMakeLists.txt for beken hardware porting")

message("ARMINO_PATH:"  ENV{ARMINO_PATH})
set(SRCS 
    src/beken_start.c 
    src/beken_irq.c 
    src/beken_uart.c 
    src/beken_timer.c 
    src/beken_arch.c
    src/beken_bringup.c
    src/beken_rtos.c
    src/beken_port.c
)
set(ENV{PYTHONPATH} $ENV{PYTHONPATH}${NUTTX_CHIP_ABS_DIR}/tools/env_tools/bk_py_libs)

message("Python:" $ENV{PYTHONPATH})

# Helper for running bk_sdk python tools with the required environment injected.
include(${CMAKE_CURRENT_LIST_DIR}/cmake/bk_python.cmake)

# Derive the Beken SoC name string from the Kconfig boolean.  Must be set
# BEFORE add_subdirectory() so subdirectories (e.g. boards/bk7258_ap/partitions)
# see it when they call bk_add_python_command().
if(CONFIG_ARCH_CHIP_BK7258)
    set(BEKEN_SOC_NAME "bk7258")
else()
    # Extend this block when additional BK SoC variants are added.
    set(BEKEN_SOC_NAME "unknown")
endif()

target_sources(arch PRIVATE ${SRCS})
target_include_directories(arch PRIVATE ${NUTTX_CHIP_ABS_DIR}/include)
add_subdirectory(common)
add_subdirectory(drivers)
add_subdirectory(soc)
add_subdirectory(component)
if (CONFIG_ARCH_CHIP_BK7258)
target_include_directories(arch PRIVATE ${NUTTX_CHIP_ABS_DIR}/bk7258_ap)
add_subdirectory(boards/bk7258_ap)
endif()

# Initialise the bk_sdk environment variables that bk_curr_project.py
# reads via os.getenv().  These variables are not set in the NuttX build
# environment, so environment.py accepts them as explicit CLI arguments and
# writes them into the process environment for any child it spawns.
# Both outputs (JSON for inspection, shell script for sourcing) are declared
# so that downstream custom commands can list them in their DEPENDS.
set(PYENV_SCRIPT ${NUTTX_CHIP_ABS_DIR}/tools/environment.py)
set(ENVIRONMENT_JSON ${CMAKE_BINARY_DIR}/env/env.json)
set(ENVIRONMENT_SH   ${CMAKE_BINARY_DIR}/env/env_setup.sh)

message("ENVIRONMENT_JSON:" ${ENVIRONMENT_JSON})
# Ensure the output directory exists before the custom command runs.
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/env)
add_custom_command(OUTPUT ${ENVIRONMENT_JSON} ${ENVIRONMENT_SH}
    COMMAND ${Python3_EXECUTABLE} ${PYENV_SCRIPT}
        --project-dir  "${NUTTX_CHIP_ABS_DIR}/app"
        --custom-project-dir  "${NUTTX_CHIP_ABS_DIR}"
        --build-dir    "${CMAKE_BINARY_DIR}"
        --soc-name     "${BEKEN_SOC_NAME}"
        --project-name "${NUTTX_CONFIG}"
        --output       "${ENVIRONMENT_JSON}"
        --export-sh    "${ENVIRONMENT_SH}"
    DEPENDS ${CONFIG_FILE_LISTS} ${PYENV_SCRIPT}
)
add_custom_target(environment_setup_helper DEPENDS ${ENVIRONMENT_JSON})
add_dependencies(arch environment_setup_helper)

# set_source_files_properties(DIRECTORY component common soc
# 	TARGET_DIRECTORY arch
# 	PROPERTIES COMPILE_OPTIONS "-Wno-undef;-Wno-strict-prototypes")
add_compile_options("-Wno-undef")
add_compile_options("-Wno-strict-prototypes")

set(project_elf_src ${CMAKE_BINARY_DIR}/project_elf_src.c)

# Generate at configure time so CMake can find the file when validating sources.
# (set_source_files_properties + GENERATED does not work reliably here because
# this directory is pulled in via add_subdirectory(... EXCLUDE_FROM_ALL).)
file(WRITE ${project_elf_src}
     "volatile const char build_version[] = __DATE__ \" \" __TIME__;\n")

# Build-time rule: regenerates the file on every build, capturing the current
# compilation timestamp via __DATE__ / __TIME__ preprocessor macros.
add_custom_command(OUTPUT ${project_elf_src}
    COMMAND sh -c "echo 'volatile const char build_version[] = __DATE__ \" \" __TIME__;' > ${project_elf_src}"
    COMMENT "armino ====> gen file ${project_elf_src}..."
    VERBATIM)
add_custom_target(_project_elf_src DEPENDS "${project_elf_src}")
add_dependencies(arch _project_elf_src)
target_sources(arch PRIVATE ${project_elf_src})






set(BK_PACKAGE_TOOL ${NUTTX_CHIP_ABS_DIR}/tools/build_tools/build_process/bk_build_package.py)
set(BK_ALL_PACKAGE ${CMAKE_BINARY_DIR}/package/all-app.bin)

bk_add_python_command(
	OUTPUT  ${BK_ALL_PACKAGE}
	SCRIPT  ${BK_PACKAGE_TOOL}
	ARGS    bk7258_ap
	DEPENDS ${CONFIG_FILE_LISTS}
	        "${CMAKE_BINARY_DIR}/bk7258/app.bin"
	        "${CMAKE_BINARY_DIR}/bk7258_ap/app.bin"
	COMMENT "Generating Post"
)
add_custom_target(bk_package DEPENDS "${BK_ALL_PACKAGE}")


set(AUTO_PARTITION_TABLE ${NUTTX_CHIP_ABS_DIR}/app/partitions/bk7258/auto_partitions.csv)
set(PARTITIONS_DIR  ${CMAKE_BINARY_DIR}/partitions)
set(auto_partition_script ${NUTTX_CHIP_ABS_DIR}/tools/build_tools/build_process/bk_build_auto_partition.py)
set(auto_partition_out ${PARTITIONS_DIR}/partitions.txt)

bk_add_python_command(
	OUTPUT  ${auto_partition_out}
	SCRIPT  ${auto_partition_script}
	ARGS    
	DEPENDS ${AUTO_PARTITION_TABLE}
	COMMENT "Generating auto_partition_out"
)
add_custom_target(_auto_partition_out DEPENDS ${auto_partition_out})
add_dependencies(arch _auto_partition_out)

set(ram_partition_script ${NUTTX_CHIP_ABS_DIR}/tools/build_tools/build_process/bk_build_ram_regions.py)
set(RAM_REGIONS_TABLE ${NUTTX_CHIP_ABS_DIR}/app/partitions/bk7258/ram_regions.csv)
set(ram_regions_out ${PARTITIONS_DIR}/ram_regions.h)


bk_add_python_command(
	OUTPUT  ${ram_regions_out}
	SCRIPT  ${ram_partition_script}
	ARGS    
	DEPENDS ${RAM_REGIONS_TABLE}
	COMMENT "Generating ram_regions_out"
)
add_custom_target(_ram_region DEPENDS ${ram_regions_out})

nuttx_cpp_compile_options(-I${PARTITIONS_DIR})
target_include_directories(arch PRIVATE ${PARTITIONS_DIR})

add_custom_target(
    nuttx_pre_build
)

add_dependencies(nuttx_pre_build _auto_partition_out _ram_region)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bk7258/)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bk7258_ap/)

message(objcopy ${CMAKE_OBJCOPY})

add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/bk7258/app.bin"
    COMMAND "cp" "${NUTTX_CHIP_ABS_DIR}/cp/components/bk_libs/bk7258/app/app.bin" "${CMAKE_BINARY_DIR}/bk7258/app.bin"
    COMMENT "Generate cp-app.bin"
    VERBATIM
)
add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/bk7258_ap/app.bin"
    COMMAND "${CMAKE_OBJCOPY}" -O binary ${CMAKE_BINARY_DIR}/nuttx "${CMAKE_BINARY_DIR}/bk7258_ap/app.bin"
    COMMENT "Generate ap-app.bin"
    DEPENDS ${CMAKE_BINARY_DIR}/nuttx
    VERBATIM
)

add_custom_target(bk_app_bin DEPENDS "${CMAKE_BINARY_DIR}/bk7258/app.bin" "${CMAKE_BINARY_DIR}/bk7258_ap/app.bin")
add_dependencies(bk_app_bin nuttx)
add_dependencies(bk_package bk_app_bin)
add_custom_target(
    nuttx_post_build ALL
    DEPENDS bk_package bk_app_bin
)
target_compile_definitions(arch PRIVATE -DNUTTX_BUILD)
target_compile_options(arch PRIVATE 
    $<$<COMPILE_LANGUAGE:C>:-mcpu=cortex-m33+nodsp -g>
    $<$<COMPILE_LANGUAGE:CXX>:-mcpu=cortex-m33+nodsp -g>)
