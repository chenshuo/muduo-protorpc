#!/usr/bin/python

import sys

import rpc
import slave_pb2

def main(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    request = slave_pb2.RunCommandRequest()
    request.command = 'sleep'
    request.args.append('8')
    request.timeout = 5
    response = slave.runCommand(None, request)
    print response
    print response.finish_time_us - response.start_time_us


if __name__ == "__main__":
    main(sys.argv[1])
