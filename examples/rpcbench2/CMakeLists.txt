PROTOBUF_GENERATE_RPC(PROTO_SRCS echo.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(echo_proto2 ${PROTO_SRCS})

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(protobuf_rpc_echo_client2 client.cc)
target_link_libraries(protobuf_rpc_echo_client2 echo_proto2 muduo_protorpc2)

add_executable(protobuf_rpc_echo_server2 server.cc)
target_link_libraries(protobuf_rpc_echo_server2 echo_proto2 muduo_protorpc2)
