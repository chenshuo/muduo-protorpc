PROTOBUF_GENERATE_RPC(PROTO_SRCS wordfreq.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(wordfreq_proto2 ${PROTO_SRCS})

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(wordfreq_worker wordfreq.cc)
target_link_libraries(wordfreq_worker wordfreq_proto2 muduo_protorpc2)

add_executable(wordfreq_control control.cc)
target_link_libraries(wordfreq_control wordfreq_proto2 muduo_protorpc2)

if(BOOSTTEST_LIBRARY)
add_executable(wordfreq_partition_unittest partition_unittest.cc)
target_link_libraries(wordfreq_partition_unittest muduo_base boost_unit_test_framework pthread)
endif()

