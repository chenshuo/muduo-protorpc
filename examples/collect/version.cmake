execute_process(
  COMMAND date "-u" "+%F %H:%M:%S"
  OUTPUT_VARIABLE build_time OUTPUT_STRIP_TRAILING_WHITESPACE)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/version.cc
     "const char* g_build_version =\"${build_time}\";\n" )

