#!/usr/bin/python

import sys

import rpc
from muduo.protorpc2 import rpc2_pb2
import master_pb2

class MasterServiceImpl(master_pb2.MasterService):
    def slaveHeartbeat(self, controller, request, done):
        print request
        return rpc2_pb2.Empty()

def main(port):
    server = rpc.ServerRpcChannel(port)
    service = MasterServiceImpl()
    while True:
        server.serveOneClient(service)

if __name__ == "__main__":
    main(int(sys.argv[1]))
