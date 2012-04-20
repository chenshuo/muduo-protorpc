
add_executable(zurg_slave main.cc
  SlaveApp.cc
  SlaveService.cc)
target_link_libraries(zurg_slave zurg_proto muduo_protorpc2)
