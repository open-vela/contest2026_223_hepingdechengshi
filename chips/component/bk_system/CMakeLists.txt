

set(incs)

if(CONFIG_FREERTOS)
	set(priv_incs os/freertos)
endif()


set(srcs
	# tick_base.c
	# printf_base.c
	# printf.c
	mac.c
	reboot.c
	delay.c
)
if(CONFIG_AT)
set(components ../../components)
list(APPEND incs
	${components}/at_server
	${components}/at_server/_at_server
	${components}/at_server/_at_server_port
	)
	list(APPEND 
		srcs
		bk_system_at.c
	)
endif()

if(CONFIG_FREERTOS)
	list(APPEND 
		srcs 
		os/freertos/tick.c
	)
elseif(CONFIG_NON_OS)
	list(APPEND
		srcs
		os/non_os/tick.c
	)
endif()

target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})
