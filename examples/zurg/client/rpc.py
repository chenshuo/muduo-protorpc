#!/usr/bin/python

import socket
import struct
import sys
import zlib

from google.protobuf import service

import rpc_pb2
import rpcservice_pb2

def encode(message):
    body = "RPC0" + message.SerializeToString()
    cksum = zlib.adler32(body)
    return "".join((struct.pack(">l", len(body) + 4), body, struct.pack(">l", cksum)))

def decode(sock):
    head = sock.recv(4, socket.MSG_WAITALL)
    if not head:
        return None
    assert len(head) == 4
    length, = struct.unpack(">l", head)
    assert length > 8
    body = sock.recv(length, socket.MSG_WAITALL)
    assert len(body) == length
    assert "RPC0" == body[:4]
    cksum, = struct.unpack(">l", body[-4:])
    cksum2 = zlib.adler32(body[:-4])
    assert cksum == cksum2
    message = rpc_pb2.RpcMessage()
    message.ParseFromString(body[4:-4])
    return message

class SyncRpcChannel(service.RpcChannel):
    def __init__(self, hostport):
        self.sock = socket.create_connection(hostport)
        self.count = 0

    def CallMethod(self, method_descriptor, rpc_controller,
                   request, response_class, done):
        message = rpc_pb2.RpcMessage()
        message.type = rpc_pb2.REQUEST
        self.count += 1
        message.id = self.count
        message.service = method_descriptor.containing_service.full_name
        message.method =  method_descriptor.name
        message.request = request.SerializeToString()
        wire = encode(message)
        self.sock.sendall(wire)
        responseMessage = decode(self.sock)
        assert responseMessage.type == rpc_pb2.RESPONSE
        assert responseMessage.id == message.id
        response = response_class()
        response.ParseFromString(responseMessage.response)
        return response

class ServerRpcChannel(service.RpcChannel):
    def __init__(self, port):
        self.serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.serversocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.serversocket.bind(('', port))
        self.serversocket.listen(5)

    def serveOneClient(self, service):
        (clientsocket, address) = self.serversocket.accept()
        print "got connection from", address
        while True:
            message = decode(clientsocket)
            if not message:
                clientsocket.close()
                break
            assert message.type == rpc_pb2.REQUEST
            assert message.service == service.GetDescriptor().full_name
            method = service.GetDescriptor().FindMethodByName(message.method)
            request_class = service.GetRequestClass(method)
            request = request_class()
            request.ParseFromString(message.request)
            response = service.CallMethod(method, None, request, None)
            responseMessage = rpc_pb2.RpcMessage()
            responseMessage.type = rpc_pb2.RESPONSE
            responseMessage.id = message.id
            responseMessage.response = response.SerializeToString()
            wire = encode(responseMessage)
            clientsocket.sendall(wire)
        print "connection is down", address

if __name__ == "__main__":
    channel = SyncRpcChannel(sys.argv[1].split(':'))
    rpcList = rpcservice_pb2.RpcService_Stub(channel)
    print rpcList.listRpc(None, rpcservice_pb2.ListRpcRequest())
    request = rpcservice_pb2.ListRpcRequest()
    request.list_method = True
    print rpcList.listRpc(None, request)
    request = rpcservice_pb2.GetServiceRequest()
    request.service_name = "zurg.SlaveService"
    print rpcList.getService(None, request)

