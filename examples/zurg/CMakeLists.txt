include_directories(${PROJECT_BINARY_DIR})

add_library(zurg_common common/ProtobufLog.cc)

add_subdirectory(proto)
add_subdirectory(slave)
