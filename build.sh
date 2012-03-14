#!/bin/sh

set -x

SOURCE_DIR=`pwd`
BUILD_DIR=${BUILD_DIR:-../build}
BUILD_TYPE=${BUILD_TYPE:-debug}
INSTALL_DIR=${INSTALL_DIR:-../protorpc-${BUILD_TYPE}-install}
MUDUO_DIR=${MUDUO_DIR:-${PWD}/$BUILD_DIR/${BUILD_TYPE}-install}

mkdir -p $BUILD_DIR/protorpc-$BUILD_TYPE\
  && cd $BUILD_DIR/protorpc-$BUILD_TYPE \
  && cmake --graphviz=dep.dot \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DMUDUO_PATH=$MUDUO_DIR $SOURCE_DIR \
  && make $*

#cd $SOURCE_DIR && doxygen

