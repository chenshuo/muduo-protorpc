PROTOBUF_GENERATE_RPC(PROTO_SRCS resolver.proto)

add_library(resolver_proto2 ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(protorpc_resolver_client client.cc)
target_link_libraries(protorpc_resolver_client resolver_proto2 muduo_protorpc2)

add_executable(protorpc_resolver_server server.cc)
target_link_libraries(protorpc_resolver_server resolver_proto2 muduo_protorpc2 muduo_cdns cares)
