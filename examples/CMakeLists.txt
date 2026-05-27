add_executable(online_example online_example.cpp)
target_include_directories(
	online_example PRIVATE ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
)
target_link_libraries(
	online_example PUBLIC
	online_data_receiver
	ROOT::Graf ROOT::Gpad
)
set_target_properties(
	online_example PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	online_example_dict ${PROJECT_SOURCE_DIR}/include/signal_handler.h
	MODULE online_example
	LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
)

add_subdirectory(ribll2026)
