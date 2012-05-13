#!/usr/bin/python

import sys

import rpc
from muduo.protorpc2 import rpc2_pb2
import master_pb2

class MasterServiceImpl(master_pb2.MasterService):
    def slaveHeartbeat(self, controller, request, done):
        print "=================", request.ByteSize()

        print "======", len(request.cpuinfo)
        print request.cpuinfo
        print "======", len(request.version)
        print request.version
        print "======", len(request.etc_mtab)
        print request.etc_mtab
        print "====== uname ", request.uname.ByteSize()
        print request.uname
        print "======", len(request.disk_usage)
        print request.disk_usage
        print "======", len(request.meminfo)
        print request.meminfo
        print "======", len(request.proc_stat)
        print request.proc_stat
        print "======", len(request.diskstats)
        print request.diskstats
        return rpc2_pb2.Empty()

def main(port):
    server = rpc.ServerRpcChannel(port)
    service = MasterServiceImpl()
    while True:
        server.serveOneClient(service)

if __name__ == "__main__":
    main(int(sys.argv[1]))
