add_executable(dssd_simulation dssd_simulation.cpp)
target_include_directories(
	dssd_simulation PRIVATE
	"${PROJECT_SOURCE_DIR}" ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(
	dssd_simulation PRIVATE
	iceoryx_binding_c::iceoryx_binding_c iceoryx_posh::iceoryx_posh
	ROOT::Core ROOT::MathCore
)
target_compile_options(
	dssd_simulation
	PRIVATE ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
)
set_target_properties(
	dssd_simulation PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)

add_executable(dssd_online dssd_online.cpp)
target_include_directories(
	dssd_online PRIVATE ${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(
	dssd_online PUBLIC
	online_data_receiver
	ROOT::Graf ROOT::Gpad
)
set_target_properties(
	dssd_online PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	dssd_online_dict ${PROJECT_SOURCE_DIR}/include/signal_handler.h
	MODULE dssd_online
	LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
)