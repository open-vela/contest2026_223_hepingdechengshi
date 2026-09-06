# BK7258 AP firmware packaging post-build step.
#
# This file is included by nuttx/CMakeLists.txt via the generic board
# post-build hook, inside the same CMake directory scope as the 'nuttx'
# target.  That means add_custom_command(TARGET nuttx POST_BUILD) is legal
# here even though this file lives in the vendor board tree.
#
# CMAKE_CURRENT_LIST_DIR resolves to this scripts/ directory at include
# time, giving a stable anchor for relative paths regardless of the build
# directory location.
#
# Layout (relative to this file):
#   scripts/ -> ../  -> bk7258_ap/
#            -> ../../  -> boards/
#            -> ../../../ -> <vendor>/   (= vendor/beken via symlink)
#            -> ../../../chips/ -> vendor/beken/chips/  ← pack script lives here

set(_bk7258_pack_script
    "${CMAKE_CURRENT_LIST_DIR}/../../../chips/tools/pack_bk7258.py")

if(NOT EXISTS "${_bk7258_pack_script}")
  message(WARNING
    "bk7258_ap post_build: packaging script not found: ${_bk7258_pack_script}")
else()
  add_custom_command(
    TARGET nuttx POST_BUILD
    COMMAND ${Python3_EXECUTABLE} "${_bk7258_pack_script}"
    COMMENT "bk7258_ap: packaging all-app.bin"
    VERBATIM)
endif()
