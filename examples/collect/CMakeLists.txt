PROTOBUF_GENERATE_RPC(PROTO_SRCS collect.proto)

add_library(collect_proto ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(collector collector.cc ProcFs.cc)
target_link_libraries(collector collect_proto muduo_protorpc2 muduo_inspect muduo_http)

add_executable(reader reader.cc)
target_link_libraries(reader collect_proto muduo_protorpc2 z)
