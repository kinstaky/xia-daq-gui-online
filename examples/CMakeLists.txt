add_executable(alpha_source_dssd_simulation alpha_source_dssd_simulation.cpp)
target_include_directories(
	alpha_source_dssd_simulation PRIVATE
	"${PROJECT_SOURCE_DIR}" ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(
	alpha_source_dssd_simulation PRIVATE
	iceoryx_binding_c::iceoryx_binding_c iceoryx_posh::iceoryx_posh
	ROOT::Core ROOT::MathCore
)
target_compile_options(
	alpha_source_dssd_simulation
	PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
)
set_target_properties(
	alpha_source_dssd_simulation PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)

add_executable(online_example online_example.cpp)
target_include_directories(
	online_example PRIVATE ${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
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

if (BUILD_FAKE_PIXIE)
	add_subdirectory(pixie_simulation)
endif()