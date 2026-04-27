file(GLOB SRC_FILES *.cc)
add_library(online SHARED ${SRC_FILES})
target_include_directories(
	online PUBLIC ${PROJECT_SOURCE_DIR} ROOT_INCLUDE_DIRS
)
target_link_libraries(
	online PUBLIC
	ROOT::Core ROOT::Gui ROOT::Graf ROOT::Gpad ROOT::Hist
	${CMAKE_DL_LIBS} rt
)
target_compile_options(
	online PUBLIC ${ICEORYX_WARNINGS} ${ICEORYX_SANITIZER_FLAGS}
)
set_target_properties(
	online PROPERTIES
	CXX_STANDARD_REQUIRED ON
	CXX_STANDARD ${ICEORYX_CXX_STANDARD}
	POSITION_INDEPENDENT_CODE ON
)
