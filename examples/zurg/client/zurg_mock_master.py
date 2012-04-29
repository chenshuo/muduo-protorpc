#!/usr/bin/python

import sys

import rpc
import master_pb2

class MasterServiceImpl(master_pb2.MasterService):
    def slaveHeartbeat(self, controller, request, done):
        print request
        return master_pb2.Empty()

def main(port):
    server = rpc.ServerRpcChannel(port)
    service = MasterServiceImpl()
    server.serveOneClient(service)

if __name__ == "__main__":
    main(int(sys.argv[1]))
