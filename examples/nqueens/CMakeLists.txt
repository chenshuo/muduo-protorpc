PROTOBUF_GENERATE_RPC(PROTO_SRCS nqueens.proto)

add_library(nqueens_proto ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(nqueens_server server.cc)
target_link_libraries(nqueens_server nqueens_proto muduo_protorpc2)

add_executable(nqueens_client client.cc)
target_link_libraries(nqueens_client nqueens_proto muduo_protorpc2)
