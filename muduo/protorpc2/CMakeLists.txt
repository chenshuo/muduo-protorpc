add_executable(protoc-gen-rpc gen.cc cpp_service.cc)
target_link_libraries(protoc-gen-rpc protoc protobuf pthread)

# FIXME: fix this command
add_custom_command(TARGET protoc-gen-rpc PRE_BUILD
  COMMAND cp
  ARGS ${Muduo_INCLUDE_DIR}/muduo/net/protorpc/rpcservice.proto rpcservice.proto
  COMMAND sed
  ARGS -i "/option cc_generic_services/d" rpcservice.proto
  VERBATIM )

set(PROTO_SRC rpcservice.proto ${CMAKE_CURRENT_SOURCE_DIR}/rpc2.proto)
add_custom_command(OUTPUT rpcservice.pb.cc rpcservice.pb.h rpc2.pb.h rpc2.pb.cc
  COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
  ARGS --plugin=${PROJECT_BINARY_DIR}/bin/protoc-gen-rpc --cpp_out . --rpc_out . ${PROTO_SRC} -I. -I${Muduo_INCLUDE_DIR}/muduo/net/protorpc -I${PROJECT_SOURCE_DIR} -I${PROTOBUF_INCLUDE_DIR}
  COMMAND sed
  ARGS -i "s/#include .rpc.pb.h./#include <muduo\\/net\\/protorpc\\/rpc.pb.h>/" rpcservice.pb.h
  COMMAND sed
  ARGS -i  "/protoc_insertion_point.includes./i#include <memory>" muduo/protorpc2/rpc2.pb.h
  COMMAND sed
  ARGS -i  "/protoc_insertion_point.namespace_scope./itypedef ::std::shared_ptr<Empty> EmptyPtr;" muduo/protorpc2/rpc2.pb.h
  COMMAND mv
  ARGS muduo/protorpc2/rpc2.pb.h muduo/protorpc2/rpc2.pb.cc .
  MAIN_DEPENDENCY ${PROTO_SRC}
  DEPENDS protoc-gen-rpc
  VERBATIM )

add_library(muduo_protorpc2 RpcChannel.cc RpcServer.cc RpcService.cc rpcservice.pb.cc rpc2.pb.cc)
target_link_libraries(muduo_protorpc2 muduo_protorpc_wire muduo_protobuf_codec muduo_net muduo_base protobuf pthread rt z)
#set_source_files_properties(../protorpc/rpc.pb.cc PROPERTIES GENERATED yes)
set_source_files_properties(rpc2.pb.cc rpcservice.pb.cc PROPERTIES COMPILE_FLAGS "-Wno-conversion")
add_dependencies(muduo_protorpc2 protoc-gen-rpc)

if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  target_link_libraries(muduo_protorpc2 tcmalloc_and_profiler)
  #target_link_libraries(muduo_protorpc2_cpp11 tcmalloc_and_profiler)
endif()

install(TARGETS protoc-gen-rpc DESTINATION bin)
install(TARGETS muduo_protorpc2 DESTINATION lib)

set(HEADERS
  RpcChannel.h
  RpcServer.h
  RpcService.h
  service.h
  ${PROJECT_BINARY_DIR}/muduo/protorpc2/rpcservice.pb.h
  )
install(FILES ${HEADERS} DESTINATION include/muduo/protorpc2)
