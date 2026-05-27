if (BUILD_GRPC_SERVER)
	add_executable(pixie_status grpc/status.cpp)
	add_executable(pixie_start grpc/start.cpp)
	add_executable(pixie_stop grpc/stop.cpp)
	foreach(target pixie_status pixie_start pixie_stop)
		target_include_directories(${target} PRIVATE "${PROJECT_SOURCE_DIR}")
		target_link_libraries(${target} PRIVATE pixie16_grpc_proto)
		set_target_properties(
			${target} PROPERTIES
			CXX_STANDARD_REQUIRED ON
			CXX_STANDARD ${ICEORYX_CXX_STANDARD}
			POSITION_INDEPENDENT_CODE ON
		)
	endforeach()
endif()
