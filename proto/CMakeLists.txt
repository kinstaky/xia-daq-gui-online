# Proto file
get_filename_component(pixie16_proto "pixie16.proto" ABSOLUTE)
get_filename_component(pixie16_proto_path "${pixie16_proto}" PATH)

# Generated sources
set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/pixie16.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/pixie16.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/pixie16.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/pixie16.grpc.pb.h")
add_custom_command(
    OUTPUT "${proto_srcs}" "${proto_hdrs}"
    "${grpc_srcs}" "${grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${pixie16_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${pixie16_proto}"
    DEPENDS "${pixie16_proto}"
)

# pixie16_grpc_proto
add_library(
	pixie16_grpc_proto
	${grpc_srcs} ${grpc_hdrs} ${proto_srcs} ${proto_hdrs}
)
target_include_directories(pixie16_grpc_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
	pixie16_grpc_proto
	PUBLIC ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF}
)
