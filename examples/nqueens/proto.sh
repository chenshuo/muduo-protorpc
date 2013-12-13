#!/bin/sh

set -x
PROTOBUF=${PROTOBUF:-/usr/local/include}
protoc --python_out=. nqueens.proto -I. -I../.. -I$PROTOBUF
protoc --go_out=go nqueens.proto -I. -I../.. -I$PROTOBUF
