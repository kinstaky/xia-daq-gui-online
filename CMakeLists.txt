# Require the cmake minimum version.
cmake_minimum_required(VERSION 3.10)


# Set project properties.
project(
	XiaDaqGui
	VERSION 1.0.1
	DESCRIPTION "A project for XIA DAQ GUI, edited from GDDAQ."
	LANGUAGES CXX C
)


# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOC_PATH)
if (EXISTS "${LOC_PATH}")
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with"
						"CmakeLists.txt file). Please make a build subdirectory. Feel free"
						"to remove CMakeCache.txt and CMakeFiles.")
endif()

# Set the build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()


# Only do these if this is the main project, and not if it is included
# through add_subdirectory
if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

	# Let's ensure -std=c++xx instead of -std=g++xx
	set(CMAKE_CXX_EXTENSIONS OFF)

	# Set the default cxx standard
	set(CMAKE_CXX_STANDARD 17)

	# Set default compile options
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

	# Let's nicely support folders in IDE's
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

endif()

# build options
option(BUILD_GRPC_SERVER "Build gRPC server" OFF)
option(BUILD_FAKE_PIXIE_TEST "Build fake Pixie test targets" OFF)

# find root
find_package(ROOT 6.20 CONFIG REQUIRED)

include(GNUInstallDirs)

# find iceoryx
find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)
find_package(iceoryx_binding_c CONFIG REQUIRED)
get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPackageHelper)
include(IceoryxPlatform)

if (BUILD_GRPC_SERVER)
	# search for grpc
	option(protobuf_MODULE_COMPATIABLE TRUE)
	find_package(Protobuf CONFIG REQUIRED)
	# set alias
	set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
	set(_REFLECTION gRPC::grpc++_reflection)
	if(CMAKE_CROSSCOMPILING)
		find_program(_PROTOBUF_PROTOC protoc)
	else()
		set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
	endif()
	# Find gRPC installation
	# Looks for gRPCConfig.cmake file installed by gRPC's cmake installation.
	find_package(gRPC CONFIG REQUIRED)
	# message(STATUS "Using gRPC ${gRPC_VERSION}")
	# set alias
	set(_GRPC_GRPCPP gRPC::grpc++)
	if(CMAKE_CROSSCOMPILING)
		find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
	else()
		set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
	endif()
endif()


# show properties
MESSAGE("Build Properties")
MESSAGE("  c++ compiler.........: " ${CMAKE_CXX_COMPILER})
MESSAGE("  c++ standard.........: " ${CMAKE_CXX_STANDARD})
MESSAGE("  cmake................: " ${CMAKE_VERSION})
MESSAGE("  build type...........: " ${CMAKE_BUILD_TYPE})
MESSAGE("  generator............: " ${CMAKE_GENERATOR})
MESSAGE("  build server.........: " ${BUILD_GRPC_SERVER})
MESSAGE("  cern ROOT version....: " ${ROOT_VERSION})
MESSAGE("  iceoryx version......: " ${iceoryx_binding_c_VERSION})
if (BUILD_GRPC_SERVER)
	MESSAGE("  protobuf.............: " ${Protobuf_VERSION})
	MESSAGE("  grpc.................: " ${gRPC_VERSION})
endif()


if (BUILD_GRPC_SERVER)
	# The protobuf files
	add_subdirectory(proto)
endif()
if (BUILD_FAKE_PIXIE_TEST)
	add_subdirectory(test/fake_pixie)
endif()
add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(bin)
add_subdirectory(examples)
if (BUILD_FAKE_PIXIE_TEST)
	add_subdirectory(test)
endif()
