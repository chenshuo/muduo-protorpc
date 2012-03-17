
add_subdirectory(rpcbench2)
add_subdirectory(sudoku)    # sync response

if(muduo_cdns)
  add_subdirectory(resolver)  # async response
else()
  add_subdirectory(resolver EXCLUDE_FROM_ALL)  # async response
endif()
