add_library(Pixie16Sys STATIC communication.c i2cm24c64.c pcf8574.c pixie16sys.c tools.c)
target_include_directories(
	Pixie16Sys PRIVATE ${PROJECT_SOURCE_DIR}/external/pixie/inc $ENV{PLX_SDK_DIR}/Include
)
target_compile_definitions(
	Pixie16Sys PRIVATE PLX_LITTLE_ENDIAN PCI_CODE PLX_LINUX
)
target_compile_options(
	Pixie16Sys PRIVATE -fcommon -fPIC $<$<CONFIG:Release>:-O0> $<$<CONFIG:Release>:-fno-strict-aliasing>
)
