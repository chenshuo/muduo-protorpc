PROTOBUF_GENERATE_RPC(PROTO_SRCS echo.proto)

add_library(echo_proto2 ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(protobuf_rpc_echo_bench2 bench.cc)
target_link_libraries(protobuf_rpc_echo_bench2 echo_proto2 muduo_protorpc2)

add_executable(protobuf_rpc_echo_client2 client.cc)
target_link_libraries(protobuf_rpc_echo_client2 echo_proto2 muduo_protorpc2)

add_executable(protobuf_rpc_echo_server2 server.cc)
target_link_libraries(protobuf_rpc_echo_server2 echo_proto2 muduo_protorpc2)
