add_executable(online_template online_template.cpp)
target_include_directories(
	online_template PRIVATE ${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(
	online_template PUBLIC
	online_data_receiver
	ROOT::Graf ROOT::Gpad
)
set_target_properties(
	online_template PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	online_template_dict ${PROJECT_SOURCE_DIR}/include/signal_handler.h
	MODULE online_template
	LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
)