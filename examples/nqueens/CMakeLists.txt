PROTOBUF_GENERATE_RPC(PROTO_SRCS nqueens.proto)

add_library(nqueens_proto2 ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(nqueens_server server.cc)
target_link_libraries(nqueens_server nqueens_proto2 muduo_protorpc2)
