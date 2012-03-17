add_custom_command(OUTPUT resolver.pb.cc resolver.pb.h
  COMMAND protoc
  ARGS --plugin=${PROJECT_BINARY_DIR}/bin/protoc-gen-rpc --rpc_out . ${CMAKE_CURRENT_SOURCE_DIR}/resolver.proto -I${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND sed
  ARGS -i "'s/#include <google\\/protobuf\\/service.h>/#include <muduo\\/protorpc2\\/service.h>/'" resolver.pb.h
  MAIN_DEPENDENCY resolver.proto
  DEPENDS protoc-gen-rpc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(resolver_proto resolver.pb.cc)

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(protorpc_resolver_client client.cc)
target_link_libraries(protorpc_resolver_client resolver_proto muduo_protorpc2)

add_executable(protorpc_resolver_server server.cc)
target_link_libraries(protorpc_resolver_server resolver_proto muduo_protorpc2 muduo_cdns cares)
