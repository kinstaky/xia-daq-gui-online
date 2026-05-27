add_executable(gddaq gddaq.cpp)
target_include_directories(
	gddaq PUBLIC ${ROOT_INCLUDE_DIRS}
)
set_target_properties(
	gddaq PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
if (BUILD_GRPC_SERVER)
	target_link_libraries(
		gddaq PRIVATE
		remote_grpc_server gui
		crypto rt fftw3
		Pixie16App Pixie16Sys
		$ENV{PLX_SDK_DIR}/PlxApi/Library/PlxApi.a ${CMAKE_DL_LIBS}
		iceoryx_posh::iceoryx_posh
	)
	add_compile_definitions(gddaq REMOTE_CONTROL_GRPC)
else()
	target_link_libraries(
		gddaq PRIVATE
		gui
		crypto rt fftw3
		Pixie16App Pixie16Sys
		$ENV{PLX_SDK_DIR}/PlxApi/Library/PlxApi.a ${CMAKE_DL_LIBS}
		iceoryx_posh::iceoryx_posh
	)
endif()
root_generate_dictionary(
	MainDict ${PROJECT_SOURCE_DIR}/external/daq/MainFrame.hh
	MODULE gddaq
	LINKDEF ${PROJECT_SOURCE_DIR}/external/daq/MainLinkdef.h
)

if (BUILD_FAKE_PIXIE_TEST)
	add_executable(gddaq_test gddaq.cpp)
	target_include_directories(
		gddaq_test PUBLIC ${ROOT_INCLUDE_DIRS}
	)
	set_target_properties(
		gddaq_test PROPERTIES
		CXX_STANDARD_REQUIRED ON
		CXX_STANDARD ${ICEORYX_CXX_STANDARD}
		POSITION_INDEPENDENT_CODE ON
	)
	if (BUILD_GRPC_SERVER)
		target_link_libraries(
			gddaq_test PRIVATE
			remote_grpc_server_test gui_test
			crypto rt fftw3
			iceoryx_posh::iceoryx_posh
		)
		add_compile_definitions(gddaq_test REMOTE_CONTROL_GRPC)
	else()
		target_link_libraries(
			gddaq_test PRIVATE
			gui_test
			crypto rt fftw3
			iceoryx_posh::iceoryx_posh
		)
	endif()
	root_generate_dictionary(
		MainTestDict ${PROJECT_SOURCE_DIR}/external/daq/MainFrame.hh
		MODULE gddaq_test
		LINKDEF ${PROJECT_SOURCE_DIR}/external/daq/MainLinkdef.h
	)
endif()
