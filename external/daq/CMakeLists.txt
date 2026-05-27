file(GLOB SRC_FILES *.cc)
add_library(gui SHARED ${SRC_FILES})
target_include_directories(
	gui PUBLIC ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
)
target_link_libraries(
	gui PUBLIC
	Pixie16App Pixie16Sys
	ROOT::Core ROOT::Gui ROOT::Graf
	iceoryx_binding_c::iceoryx_binding_c
	stdc++fs
)
target_compile_options(
	gui PUBLIC -fno-stack-protector
	${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
	$<$<CONFIG:Release>:-O0> $<$<CONFIG:Release>:-fno-strict-aliasing>
)
set_target_properties(
	gui PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)

if (BUILD_FAKE_PIXIE_TEST)
	add_library(gui_test SHARED ${SRC_FILES})
	target_include_directories(
		gui_test PUBLIC ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
	)
	target_link_libraries(
		gui_test PUBLIC
		fake_pixie
		ROOT::Core ROOT::Gui ROOT::Graf
		iceoryx_binding_c::iceoryx_binding_c
		stdc++fs
	)
	target_compile_options(
		gui_test PUBLIC -fno-stack-protector
		${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
		$<$<CONFIG:Release>:-O0> $<$<CONFIG:Release>:-fno-strict-aliasing>
	)
	set_target_properties(
		gui_test PROPERTIES
		CXX_STANDARD_REQUIRED ON
		CXX_STANDARD ${ICEORYX_CXX_STANDARD}
		POSITION_INDEPENDENT_CODE ON
	)
endif()
