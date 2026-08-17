set(incs include)

set(srcs
	pm.c
	pm_model.c
	low_vol.c
	pm_misc.c
	)

target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})
