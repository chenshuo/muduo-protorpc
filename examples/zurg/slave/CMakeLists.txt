#file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/SlaveVersion.cc "const char* slave_version;" )
execute_process(COMMAND date "-u" "+%F %H:%M" OUTPUT_VARIABLE build_time OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND echo "const char* slave_version =" \"${build_time}\" ";"
  OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/SlaveVersion.cc
  )

add_executable(zurg_slave main.cc
  AppManager.cc
  ChildManager.cc
  Heartbeat.cc
  Process.cc
  RpcClient.cc
  SlaveApp.cc
  SlaveService.cc
  SlaveVersion.cc
  )

target_link_libraries(zurg_slave zurg_common zurg_proto muduo_protorpc2)
