PROTOBUF_GENERATE_RPC(PROTO_SRCS collect.proto)

add_library(collect_proto ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")

add_custom_target(
  GenerateBuildTimestamp
  ALL
  COMMAND
  ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
  )

add_executable(collector collector.cc ProcFs.cc version.cc)
target_link_libraries(collector collect_proto muduo_protorpc2 muduo_inspect muduo_http)
add_dependencies(collector GenerateBuildTimestamp)
set_source_files_properties(version.cc PROPERTIES GENERATED true)

add_executable(reader reader.cc)
target_link_libraries(reader collect_proto muduo_protorpc2 z)
