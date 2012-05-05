#!/usr/bin/python

import sys

import rpc
import slave_pb2
import master_pb2

def getFile(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.GetFileContentRequest()
    request.file_name = '/etc/issue'
    response = slave.getFileContent(None, request)
    print response

def fileChecksum(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.GetFileChecksumRequest()
    request.files.append('/bin/bash')
    request.files.append('/var')
    request.files.append('/bin/sh')
    response = slave.getFileChecksum(None, request)
    print response

def runCommand(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.RunCommandRequest()
    request.command = 'pwd'
    request.args.append('8')
    request.timeout = 5
    response = slave.runCommand(None, request)
    print response
    print response.finish_time_us - response.start_time_us

def runScript(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.RunScriptRequest()
    request.script = """#!/usr/bin/python
import sys
print sys.argv
"""
    response = slave.runScript(None, request)
    print response

def getHardware(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.GetHardwareRequest()
    request.lshw = False
    response = slave.getHardware(None, request)
    print response.lspci
    print response.lscpu
    print response.lshw
    print response.ifconfig

def addApplication(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.AddApplicationRequest()
    request.name = 'xyz'
    request.binary = 'test1.py'
    response = slave.addApplication(None, request)
    print response
    request = slave_pb2.StartApplicationsRequest();
    request.names.append('xyz')
    response = slave.startApplications(None, request)
    print response

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
    addApplication(sys.argv[1])
