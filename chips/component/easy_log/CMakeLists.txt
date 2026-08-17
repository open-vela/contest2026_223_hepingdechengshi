set(incs include)
set(srcs "")

list(APPEND srcs
	src/easy_log.c
)



target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})