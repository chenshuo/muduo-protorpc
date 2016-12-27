function(PROTOBUF_GENERATE_RPC SRCS)
  if(NOT ARGN)
    message(SEND_ERROR "Error: PROTOBUF_GENERATE_RPC() called without any proto files")
    return()
  endif(NOT ARGN)

  set(${SRCS})
  foreach(FIL ${ARGN})

    get_filename_component(FIL_WE ${FIL} NAME_WE)
    list(APPEND ${SRCS} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.pb.cc")

    add_custom_command(
      OUTPUT "${FIL_WE}.pb.cc" "${FIL_WE}.pb.h"
      COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
      ARGS --plugin=${PROJECT_BINARY_DIR}/bin/protoc-gen-rpc --cpp_out . --rpc_out . ${CMAKE_CURRENT_SOURCE_DIR}/${FIL} -I${CMAKE_CURRENT_SOURCE_DIR} -I${PROJECT_SOURCE_DIR} -I${PROTOBUF_INCLUDE_DIR}
      MAIN_DEPENDENCY ${FIL}
      DEPENDS protoc-gen-rpc
      VERBATIM )
  endforeach()
  set(${SRCS} ${${SRCS}} PARENT_SCOPE)
endfunction()

add_subdirectory(rpcbench2)
add_subdirectory(sudoku)    # sync response

if(muduo_cdns)
  add_subdirectory(resolver)  # async response
else()
  add_subdirectory(resolver EXCLUDE_FROM_ALL)  # async response
endif()

add_subdirectory(collect)
add_subdirectory(median)
add_subdirectory(nqueens)
add_subdirectory(wordfreq)
add_subdirectory(zurg)
