set(incs include include/bk_private)
set(srcs)
#if(CONFIG_WIFI_ENABLE)
list(APPEND srcs
	src/wifi_api.c
	# src/wifi_cli.c
	src/wifi_api_ipc.c
	src/phy_client.c
	src/phy_notify.c
	# src/wifi_demo.c
)
#endif()


if(CONFIG_AT)
list(APPEND srcs
	src/wifi_at.c
)
set(components ../../components)
set(IDK_INC ../../include)
list(APPEND incs
	${components}/at_server
	${components}/at_server/_at_server
	${components}/at_server/_at_server_port
	${components}/bk_net/include
	${components}/bk_event
	${components}/bk_wifi_driver
	${IDK_INC}/modules
	)
endif()

if(CONFIG_NET_PARAM)
list(APPEND srcs
	src/net_param.c
)
endif()



target_sources(arch PRIVATE ${srcs})
target_include_directories(arch PRIVATE ${incs})
set_source_files_properties(${srcs}
	TARGET_DIRECTORY arch
	PROPERTIES COMPILE_OPTIONS "-Wno-undef;-Wno-strict-prototypes")
