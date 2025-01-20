add_library(online_data_receiver online_data_receiver.cpp)
target_include_directories(
	online_data_receiver PUBLIC ${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(
	online_data_receiver PUBLIC
	iceoryx_binding_c::iceoryx_binding_c
)
target_compile_options(
	online_data_receiver PUBLIC ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
)
set_target_properties(
	online_data_receiver PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)

if (BUILD_GRPC_SERVER)
	add_library(remote_grpc_server remote_grpc_server.cpp)
	target_include_directories(remote_grpc_server PUBLIC "${PROJECT_SOURCE_DIR}")
	target_link_libraries(remote_grpc_server PUBLIC pixie16_rong_grpc_proto gui)
endif()