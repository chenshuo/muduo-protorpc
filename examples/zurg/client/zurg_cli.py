#!/usr/bin/python

import sys

import rpc
import slave_pb2
import master_pb2

def runCommand(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.RunCommandRequest()
    request.command = 'sleep'
    request.args.append('8')
    request.timeout = 5
    response = slave.runCommand(None, request)
    print response
    print response.finish_time_us - response.start_time_us

def getHardware(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.GetHardwareRequest()
    request.lshw = True
    response = slave.getHardware(None, request)
    print response.lspci
    print response.lscpu
    print response.lshw
    print response.ifconfig

def main2(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    master = master_pb2.MasterService_Stub(channel)
    request = master_pb2.SlaveHeartbeat()
    request.slave_name = 'sleep'
    request.host_name = 'atom'
    request.slave_pid = 123
    request.start_time = 5
    response = master.slaveHeartbeat(None, request)
    print response

if __name__ == "__main__":
    getHardware(sys.argv[1])
