PROTOBUF_GENERATE_RPC(PROTO_SRCS slave.proto master.proto)

add_library(zurg_proto ${PROTO_SRCS})
set_source_files_properties(${PROTO_SRCS} PROPERTIES COMPILE_FLAGS "-Wno-conversion")
