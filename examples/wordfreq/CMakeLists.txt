PROTOBUF_GENERATE_RPC(PROTO_SRCS wordfreq.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(wordfreq_proto2 ${PROTO_SRCS})

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(protobuf_rpc_wordfreq wordfreq.cc)
target_link_libraries(protobuf_rpc_wordfreq wordfreq_proto2 muduo_protorpc2)
