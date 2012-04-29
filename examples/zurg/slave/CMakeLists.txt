add_executable(zurg_slave main.cc
  ChildManager.cc
  Heartbeat.cc
  Process.cc
  SlaveApp.cc
  SlaveService.cc)

target_link_libraries(zurg_slave zurg_common zurg_proto muduo_protorpc2)
