#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install-cpp11}
MUDUO_DIR=${MUDUO_DIR:-${PWD}/$BUILD_DIR/${BUILD_TYPE}-install-cpp11}

mkdir -p $BUILD_DIR/protorpc-$BUILD_TYPE\
  && cd $BUILD_DIR/protorpc-$BUILD_TYPE \
  && cmake \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DMUDUO_PATH=$MUDUO_DIR $SOURCE_DIR \
  && make $*

#cd $SOURCE_DIR && doxygen

