PROTOBUF_GENERATE_RPC(PROTO_SRCS resolver.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(resolver_proto ${PROTO_SRCS})

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(protorpc_resolver_client client.cc)
target_link_libraries(protorpc_resolver_client resolver_proto muduo_protorpc2)

add_executable(protorpc_resolver_server server.cc)
target_link_libraries(protorpc_resolver_server resolver_proto muduo_protorpc2 muduo_cdns cares)
