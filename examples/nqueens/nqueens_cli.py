#!/usr/bin/python

import sys
import rpc
from muduo.protorpc2 import rpc2_pb2
import nqueens_pb2

channel = rpc.SyncRpcChannel(('localhost', 9352))
server = nqueens_pb2.NQueensService_Stub(channel)

if len(sys.argv) < 3:
	print "Usage: nqueens_cli.py nqueens first_row [second_row]"
else:
	request = nqueens_pb2.SubProblemRequest()
	request.nqueens = int(sys.argv[1])
	request.first_row = int(sys.argv[2])
	if len(sys.argv) >= 4:
		request.second_row = int(sys.argv[3])
	print server.Solve(None, request)


