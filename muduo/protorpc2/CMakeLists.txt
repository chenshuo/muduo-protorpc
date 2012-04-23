add_executable(protoc-gen-rpc gen.cc)
target_link_libraries(protoc-gen-rpc protoc protobuf pthread)

set(PROTO_SRC ${Muduo_INCLUDE_DIR}/muduo/net/protorpc/rpcservice.proto)
add_custom_command(OUTPUT rpcservice.pb.cc rpcservice.pb.h
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
  ARGS --plugin=${PROJECT_BINARY_DIR}/bin/protoc-gen-rpc --rpc_out . ${PROTO_SRC} -I${Muduo_INCLUDE_DIR}/muduo/net/protorpc -I${PROTOBUF_INCLUDE_DIR}
  COMMAND sed
  ARGS -i "s/#include <google\\/protobuf\\/service.h>/#include <muduo\\/protorpc2\\/service.h>/" rpcservice.pb.h
  COMMAND sed
  ARGS -i "s/#include .rpc.pb.h./#include <muduo\\/net\\/protorpc\\/rpc.pb.h>/" rpcservice.pb.h
  MAIN_DEPENDENCY ${PROTO_SRC}
  DEPENDS protoc-gen-rpc
  VERBATIM )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(muduo_protorpc2 RpcChannel.cc RpcServer.cc RpcService.cc rpcservice.pb.cc)
target_link_libraries(muduo_protorpc2 muduo_protorpc muduo_net muduo_base protobuf pthread)
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
