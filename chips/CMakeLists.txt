# Beken custom-chip dispatcher.
#
# Keep the chip-specific entry point in chips/bk7258.  Shared Beken drivers
# and SDK components remain siblings of that directory and are added by the
# selected chip CMake file.

if(CONFIG_ARCH_CHIP_BK7258)
  add_subdirectory(bk7258)
endif()
