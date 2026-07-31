set(incs include)
set(srcs
	src/avdk_crc.c
)

set(priv_req )
target_include_directories(arch PRIVATE ${incs})
target_sources(arch PRIVATE ${srcs})



