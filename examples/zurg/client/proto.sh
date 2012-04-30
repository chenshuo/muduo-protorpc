#!/bin/sh

set -x
MUDUO=${MUDUO:-../../../../build/debug-install/include/muduo/net/protorpc}
PROTOBUF=${PROTOBUF:-/usr/local/include}
protoc --python_out=. ../../../muduo/protorpc2/rpc2.proto -I../../.. -I$PROTOBUF
protoc --python_out=. ../proto/slave.proto ../proto/master.proto -I../proto -I../../.. -I$PROTOBUF
protoc --python_out=. $MUDUO/*.proto -I$MUDUO
