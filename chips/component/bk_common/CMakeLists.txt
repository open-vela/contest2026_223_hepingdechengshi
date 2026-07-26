set(incs include)
set(srcs bk_feature.c)

target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})
set_source_files_properties(${srcs}
	TARGET_DIRECTORY arch
	PROPERTIES COMPILE_OPTIONS "-Wno-undef;-Wno-strict-prototypes")