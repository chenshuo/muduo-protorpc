PROTOBUF_GENERATE_RPC(PROTO_SRCS collect.proto)

add_library(collect_proto ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(collector collector.cc)
target_link_libraries(collector collect_proto muduo_protorpc2 muduo_inspect muduo_http)
