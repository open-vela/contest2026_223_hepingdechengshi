message("Beken APP add subdirectory Parent:" ${CMAKE_PARENT_LIST_FILE})
add_subdirectory(app)
nuttx_generate_kconfig(MENUDESC "Beken Projects")