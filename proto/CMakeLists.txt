# Proto file
get_filename_component(pixie16_rong_proto "pixie16_rong.proto" ABSOLUTE)
get_filename_component(pixie16_rong_proto_path "${pixie16_rong_proto}" PATH)

# Generated sources
set(proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/pixie16_rong.pb.cc")
set(proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/pixie16_rong.pb.h")
set(grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/pixie16_rong.grpc.pb.cc")
set(grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/pixie16_rong.grpc.pb.h")
add_custom_command(
    OUTPUT "${proto_srcs}" "${proto_hdrs}"
    "${grpc_srcs}" "${grpc_hdrs}"
    COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${pixie16_rong_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${pixie16_rong_proto}"
    DEPENDS "${pixie16_rong_proto}"
)

# pixie16_rong_grpc_proto
add_library(
	pixie16_rong_grpc_proto
	${grpc_srcs} ${grpc_hdrs} ${proto_srcs} ${proto_hdrs}
)
target_include_directories(pixie16_rong_grpc_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
	pixie16_rong_grpc_proto
	PUBLIC ${_REFLECTION} ${_GRPC_GRPCPP} ${_PROTOBUF_LIBPROTOBUF}
)