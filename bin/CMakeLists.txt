add_executable(gddaq gddaq.cpp)
target_include_directories(
	gddaq PUBLIC ROOT_INCLUDE_DIRS
)
target_link_libraries(
	gddaq PRIVATE
	gui
	crypto rt fftw3
	Pixie16App Pixie16Sys
	$ENV{PLX_SDK_DIR}/PlxApi/Library/PlxApi.a ${CMAKE_DL_LIBS}
	iceoryx_posh::iceoryx_posh
)
set_target_properties(
	gddaq PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	MainDict ${PROJECT_SOURCE_DIR}/external/daq/MainFrame.hh
	MODULE gddaq
	LINKDEF ${PROJECT_SOURCE_DIR}/external/daq/MainLinkdef.h
)

add_executable(online_statistics online_statistics.cpp)
target_include_directories(
	online_statistics PUBLIC ROOT_INCLUDE_DIRS
)
target_link_libraries(
	online_statistics PRIVATE
	online
)
set_target_properties(
	online_statistics PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
root_generate_dictionary(
	OnlineStatisticsDict ${PROJECT_SOURCE_DIR}/external/online/Online.hh
	MODULE online_statistics
	LINKDEF ${PROJECT_SOURCE_DIR}/external/online/OnlineLinkdef.h
)
