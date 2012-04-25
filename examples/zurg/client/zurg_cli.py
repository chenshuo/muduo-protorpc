#!/usr/bin/python

import sys

import rpc
import slave_pb2

def main(addr):
    channel = rpc.SyncRpcChannel(addr.split(':'))
    slave = slave_pb2.SlaveService_Stub(channel)
    slave.listRpc


if __name__ == "__main__":
    main()
