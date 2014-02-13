add_library(zurg_common common/ProtobufLog.cc common/Util.cc)

add_subdirectory(proto)
add_subdirectory(slave)

add_executable(dummy_load dummyload/dummyload.cc)
