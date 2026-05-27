function(add_ribll2026_example target_name source_file)
	add_executable(${target_name} ${source_file})
	target_include_directories(
		${target_name} PRIVATE ${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
	)
	target_link_libraries(
		${target_name} PUBLIC
		online_data_receiver
		ROOT::Graf ROOT::Gpad
	)
	set_target_properties(
		${target_name} PROPERTIES
		CXX_STANDARD_REQUIRED ON
		CXX_STANDARD ${ICEORYX_CXX_STANDARD}
		POSITION_INDEPENDENT_CODE ON
	)
	root_generate_dictionary(
		${target_name}_dict ${PROJECT_SOURCE_DIR}/include/signal_handler.h
		MODULE ${target_name}
		LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
	)
endfunction()

add_ribll2026_example(xia0 xia0.cpp)
add_ribll2026_example(xia1 xia1.cpp)
add_ribll2026_example(xia2 xia2.cpp)
