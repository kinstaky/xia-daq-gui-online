add_subdirectory(daq)
if (NOT BUILD_FAKE_PIXIE)
	add_subdirectory(pixie)
endif()