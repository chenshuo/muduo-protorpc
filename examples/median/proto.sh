#!/bin/sh

set -x
PROTOBUF=${PROTOBUF:-/usr/local/include}
protoc --python_out=. -I. median.proto -I../.. -I$PROTOBUF
