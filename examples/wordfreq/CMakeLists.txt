PROTOBUF_GENERATE_RPC(PROTO_SRCS wordfreq.proto)

add_library(wordfreq_proto ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_executable(wordfreq_worker wordfreq.cc)
target_link_libraries(wordfreq_worker wordfreq_proto muduo_protorpc2)

add_executable(wordfreq_control control.cc)
target_link_libraries(wordfreq_control wordfreq_proto muduo_protorpc2)

if(BOOSTTEST_LIBRARY)
add_executable(wordfreq_partition_unittest partition_unittest.cc)
target_link_libraries(wordfreq_partition_unittest muduo_base boost_unit_test_framework pthread)
endif()

