cmake_minimum_required(VERSION 2.8)

project(muduo-protorpc CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "debug")
endif()

if(NOT MUDUO_PATH)
  set(MUDUO_PATH "/home/$ENV{USER}/build/${CMAKE_BUILD_TYPE}-install")
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
 -Wshadow
 -Wwrite-strings
 -march=native
 # -MMD
 # -std=c++0x
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

include_directories(${Muduo_INCLUDE_DIR})
find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)
find_library(muduo_protorpc muduo_protorpc)
find_library(muduo_cdns muduo_cdns)
message(STATUS ${muduo_base})
message(STATUS ${muduo_net})
message(STATUS ${muduo_protorpc})
message(STATUS ${muduo_cdns})
link_directories(${Muduo_LIBRARY_DIR})

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
find_library(BOOSTTEST_LIBRARY NAMES boost_unit_test_framework)

find_package(Protobuf REQUIRED)
find_path(PROTOBUF_PLUGIN google/protobuf/compiler/cpp/cpp_generator.h)

include_directories(${PROJECT_SOURCE_DIR})

if(NOT muduo_protorpc)
  message(SEND_ERROR "Cannot find libmuduo_protorpc, did you install protobuf?")
endif()

add_subdirectory(muduo/protorpc2)

add_subdirectory(examples)

