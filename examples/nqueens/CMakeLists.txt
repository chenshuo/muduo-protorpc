PROTOBUF_GENERATE_RPC(PROTO_SRCS nqueens.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(nqueens_proto2 ${PROTO_SRCS})

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(nqueens_server server.cc)
target_link_libraries(nqueens_server nqueens_proto2 muduo_protorpc2)
