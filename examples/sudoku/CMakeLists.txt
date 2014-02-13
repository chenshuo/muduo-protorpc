PROTOBUF_GENERATE_RPC(PROTO_SRCS sudoku.proto)

add_library(sudoku_proto2 ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(protobuf_rpc_sudoku_client2 client.cc)
target_link_libraries(protobuf_rpc_sudoku_client2 sudoku_proto2 muduo_protorpc2)

add_executable(protobuf_rpc_sudoku_server2 server.cc)
target_link_libraries(protobuf_rpc_sudoku_server2 sudoku_proto2 muduo_protorpc2)
