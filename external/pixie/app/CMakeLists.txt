add_library(Pixie16App STATIC pixie16app.c utilities.c)
target_include_directories(
	Pixie16App PRIVATE ${PROJECT_SOURCE_DIR}/external/pixie/inc ${PROJECT_SOURCE_DIR}/external/pixie/sys
)
target_compile_definitions(
	Pixie16App PRIVATE PLX_LITTLE_ENDIAN PCI_CODE PLX_LINUX
)
target_compile_options(
	Pixie16App PRIVATE -fcommon -fPIC $<$<CONFIG:Release>:-O0> $<$<CONFIG:Release>:-fno-strict-aliasing>
)