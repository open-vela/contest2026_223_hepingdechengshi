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

set_source_files_properties(DIRECTORY component
	TARGET_DIRECTORY arch
	PROPERTIES COMPILE_OPTIONS "-Wno-undef;-Wno-strict-prototypes")
