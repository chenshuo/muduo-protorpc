#!/usr/bin/python

import sys
import rpc
from muduo.protorpc2 import rpc2_pb2
import median_pb2

channel = rpc.SyncRpcChannel(('localhost', 5555))
sorter = median_pb2.Sorter_Stub(channel)

if len(sys.argv) < 2:
	print "Usage:"
elif sys.argv[1] == 'g':
	request = median_pb2.GenerateRequest()
	request.count = 11
	request.min = 0
	request.max = 10
	print sorter.Generate(None, request)
elif sys.argv[1] == 'q':
	print sorter.Query(None, rpc2_pb2.Empty())
elif sys.argv[1] == 's':
	request = median_pb2.SearchRequest()
	request.guess = int(sys.argv[2])
	print sorter.Search(None, request)
else:
	print "Usage:"


