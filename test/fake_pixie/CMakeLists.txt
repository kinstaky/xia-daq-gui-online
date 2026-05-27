add_library(fake_pixie STATIC fake_pixie.cpp)
target_include_directories(
	fake_pixie
	PUBLIC
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/external/pixie/inc
	${PROJECT_SOURCE_DIR}/external/pixie/app
	${PROJECT_SOURCE_DIR}/external/pixie/sys
)
target_compile_definitions(
	fake_pixie PRIVATE PLX_LITTLE_ENDIAN PCI_CODE PLX_LINUX
)
target_compile_options(
	fake_pixie PRIVATE -fcommon -fPIC
	${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
	$<$<CONFIG:Release>:-O0> $<$<CONFIG:Release>:-fno-strict-aliasing>
)
set_target_properties(
	fake_pixie PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
