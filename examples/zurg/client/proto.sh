#!/bin/sh

set -x
MUDUO=${MUDUO:-../../../../build/debug-install/include/muduo/net/protorpc}
protoc --python_out=. ../proto/slave.proto ../proto/master.proto -I../proto
protoc --python_out=. $MUDUO/*.proto -I$MUDUO
