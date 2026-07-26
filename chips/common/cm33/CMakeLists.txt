
set(incs)
set(srcs)

if("${CONFIG_SOC_ARCH}" STREQUAL "cm33")
	list(APPEND incs
		include
	)

	list(APPEND srcs
		stack_base.c
		trap_base_smp.c
		arm.c
		fpb.c
		dwt.c
		arch_interrupt.c
		jump.c
		arch_delay.c
	)

if(CONFIG_DUMP_TEST)
	list(APPEND srcs
		trap_test.c
	)
endif()


if(CONFIG_MPU)
	message("CM33 MPU Enable")
	list(APPEND srcs
		mpu.c
	)
endif()

if(CONFIG_CACHE_ENABLE)
	list(APPEND srcs
		cache.c
	)
endif()

if(CONFIG_FREERTOS)
	list(APPEND incs os/freertos ./)
	list(APPEND srcs os/freertos/cpu.c)
elseif(CONFIG_LITEOS_M)
	list(APPEND incs os/liteos_m)
	list(APPEND srcs os/liteos_m/boot_handlers.S os/liteos_m/cpu.c)
elseif(CONFIG_NON_OS)
	list(APPEND incs os/non_os)
	list(APPEND srcs os/non_os/boot_handlers.S os/non_os/cpu.c)
endif()
endif()

target_include_directories(arch PRIVATE ${incs})
target_sources(arch PRIVATE ${srcs})
set_source_files_properties(${srcs}
	TARGET_DIRECTORY arch
	PROPERTIES COMPILE_OPTIONS "-Wno-undef;-Wno-strict-prototypes")

