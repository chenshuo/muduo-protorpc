add_executable(protoc-gen-rpc gen.cc)
target_link_libraries(protoc-gen-rpc protoc protobuf pthread)

set(PROTO_SRC ${Muduo_INCLUDE_DIR}/muduo/net/protorpc/rpcservice.proto ${CMAKE_CURRENT_SOURCE_DIR}/rpc2.proto)
add_custom_command(OUTPUT rpcservice.pb.cc rpcservice.pb.h rpc2.pb.h rpc2.pb.cc
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
  ARGS --plugin=${PROJECT_BINARY_DIR}/bin/protoc-gen-rpc --rpc_out . ${PROTO_SRC} -I${Muduo_INCLUDE_DIR}/muduo/net/protorpc -I${PROJECT_SOURCE_DIR} -I${PROTOBUF_INCLUDE_DIR}
  COMMAND sed
  ARGS -i "s/#include <google\\/protobuf\\/service.h>/#include <muduo\\/protorpc2\\/service.h>/" rpcservice.pb.h
  COMMAND sed
  ARGS -i "s/#include .rpc.pb.h./#include <muduo\\/net\\/protorpc\\/rpc.pb.h>/" rpcservice.pb.h
  COMMAND sed
  ARGS -i  "/protoc_insertion_point.includes./i#include <boost/shared_ptr.hpp>" muduo/protorpc2/rpc2.pb.h
  COMMAND mv
  ARGS muduo/protorpc2/rpc2.pb.h muduo/protorpc2/rpc2.pb.cc .
  MAIN_DEPENDENCY ${PROTO_SRC}
  DEPENDS protoc-gen-rpc
  VERBATIM )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(muduo_protorpc2 RpcChannel.cc RpcServer.cc RpcService.cc rpcservice.pb.cc rpc2.pb.cc)
target_link_libraries(muduo_protorpc2 muduo_protorpc muduo_net muduo_base protobuf pthread z)
#set_source_files_properties(../protorpc/rpc.pb.cc PROPERTIES GENERATED yes)
add_dependencies(muduo_protorpc2 protoc-gen-rpc)

install(TARGETS protoc-gen-rpc DESTINATION bin)
install(TARGETS muduo_protorpc2 DESTINATION lib)
set(HEADERS
  RpcChannel.h
  RpcServer.h
  RpcService.h
  )
install(FILES ${HEADERS} DESTINATION include/muduo/protorpc2)
