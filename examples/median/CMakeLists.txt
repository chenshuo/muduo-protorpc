PROTOBUF_GENERATE_RPC(PROTO_SRCS median.proto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion -Wno-extra")
include_directories(${PROJECT_BINARY_DIR})

add_library(median_proto ${PROTO_SRCS})

#TODO set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=conversion")

add_executable(median_sorter sorter.cc)
target_link_libraries(median_sorter median_proto muduo_protorpc2)

add_executable(median_collector collector.cc)
target_link_libraries(median_collector median_proto muduo_protorpc2)

if(BOOSTTEST_LIBRARY)
add_executable(median_kth_unittest kth_unittest.cc)
target_link_libraries(median_kth_unittest muduo_base boost_unit_test_framework pthread)
endif()

