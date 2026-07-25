add_library(alpha_source alpha_source.cpp)
target_include_directories(alpha_source PUBLIC ${PROJECT_SOURCE_DIR})

add_library(detector detector.cpp)
target_link_libraries(detector PUBLIC alpha_source)

add_library(fake_pixie_service fake_pixie_service.cpp)
target_link_libraries(fake_pixie_service PUBLIC detector)
target_compile_options(fake_pixie_service PRIVATE -fcommon -fPIC)

add_library(fake_pixie fake_pixie.cpp)
target_include_directories(fake_pixie PUBLIC "${PROJECT_SOURCE_DIR}")
target_link_libraries(fake_pixie PUBLIC fake_pixie_service)
target_compile_options(fake_pixie PRIVATE -fcommon -fPIC)

add_executable(fake_online fake_online.cpp)
target_include_directories(
	fake_online PRIVATE ${PROJECT_SOURCE_DIR} ${ROOT_INCLUDE_DIRS}
)
target_link_libraries(
	fake_online PUBLIC
	online_data_receiver
	ROOT::Graf ROOT::Gpad
)
set_target_properties(
	fake_online PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	fake_online_dict ${PROJECT_SOURCE_DIR}/include/signal_handler.h
	MODULE fake_online
	LINKDEF ${PROJECT_SOURCE_DIR}/include/linkdef.h
)