cmake_minimum_required(VERSION 2.8)

project(muduo-protorpc C CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT MUDUO_PATH)
  set(MUDUO_PATH "/home/$ENV{USER}/build/${CMAKE_BUILD_TYPE}-install-cpp11")
endif()

set(CXX_FLAGS
 -g
 # -DVALGRIND
 # -DMUDUO_STD_STRING
 -Wall
 -Wextra
 # -m32
 -Werror
 -Wconversion
 -Wno-unused-parameter
 -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wno-error=shadow
 -Wwrite-strings
 -march=native
 # -MMD
 -std=c++11
 )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_path(Muduo_INCLUDE_DIR "muduo/base/Types.h" "${MUDUO_PATH}/include")
find_path(Muduo_LIBRARY_DIR libmuduo_net.a "${MUDUO_PATH}/lib")
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${Muduo_LIBRARY_DIR})
message(STATUS "Muduo include " ${Muduo_INCLUDE_DIR})
message(STATUS "Muduo lib " ${Muduo_LIBRARY_DIR})

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_BINARY_DIR})

include_directories(${Muduo_INCLUDE_DIR})
find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)
find_library(muduo_protorpc_wire muduo_protorpc_wire)
find_library(muduo_cdns muduo_cdns)
message(STATUS ${muduo_base})
message(STATUS ${muduo_net})
message(STATUS ${muduo_protorpc_wire})
message(STATUS ${muduo_cdns})
link_directories(${Muduo_LIBRARY_DIR})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)

find_package(Protobuf REQUIRED)
find_path(PROTOBUF_PLUGIN google/protobuf/compiler/cpp/cpp_generator.h)

find_path(TCMALLOC_INCLUDE_DIR google/heap-profiler.h)
find_library(TCMALLOC_LIBRARY NAMES tcmalloc_and_profiler)
if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  message(STATUS "found tcmalloc")
else()
  message(STATUS "not found tcmalloc")
endif()

if(NOT muduo_protorpc_wire)
  message(SEND_ERROR "Cannot find libmuduo_protorpc_wire, did you install protobuf?")
endif()

add_subdirectory(muduo/protorpc2)

add_subdirectory(examples)

