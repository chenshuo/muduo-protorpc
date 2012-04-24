package com.chenshuo.muduo.protorpc;

import java.net.InetSocketAddress;

import com.chenshuo.muduo.protorpc.RpcServiceProto.GetServiceRequest;
import com.chenshuo.muduo.protorpc.RpcServiceProto.GetServiceResponse;
import com.chenshuo.muduo.protorpc.RpcServiceProto.ListRpcRequest;
import com.chenshuo.muduo.protorpc.RpcServiceProto.ListRpcResponse;
import com.chenshuo.muduo.protorpc.RpcServiceProto.RpcService;

public class RpcList {

    private RpcClient client;
    private RpcChannel channel;
    RpcService.BlockingInterface rpcService;

    public RpcList(InetSocketAddress addr) {
        client = new RpcClient();
        channel = client.blockingConnect(addr);
        rpcService = RpcService.newBlockingStub(channel);
    }

    public void close() {
        channel.disconnect();
        client.stop();
    }

    public void listRpc(boolean listMethod) throws Exception {
        ListRpcRequest request = ListRpcRequest.newBuilder().setListMethod(listMethod).build();
        ListRpcResponse response = rpcService.listRpc(null, request);
        System.out.println(response);
    }

    public void listOneService(String service, boolean listMethod) throws Exception {
        ListRpcRequest request = ListRpcRequest.newBuilder().setServiceName(service).setListMethod(listMethod).build();
        ListRpcResponse response = rpcService.listRpc(null, request);
        System.out.println(response);
    }

    public void getService(String service) throws Exception {
        GetServiceRequest request = GetServiceRequest.newBuilder().setServiceName(service).build();
        GetServiceResponse response = rpcService.getService(null, request);
        System.out.println(response.getError());
        for (int i = 0; i < response.getProtoFileCount(); ++i) {
            System.out.println(response.getProtoFileName(i));
            System.out.println(response.getProtoFile(i));
        }
    }

    public static void main(String[] args) throws Exception {
        InetSocketAddress addr = new InetSocketAddress(args[0], Integer.parseInt(args[1]));
        RpcList rpcList = new RpcList(addr);
        rpcList.listRpc(false);
        rpcList.listRpc(true);
        rpcList.listOneService("xxx", false);
        rpcList.listOneService("zurg.SlaveService", false);
        rpcList.listOneService("zurg.SlaveService", true);
        rpcList.getService("zurg.SlaveService");
        rpcList.getService("muduo.net.RpcService");
        rpcList.close();
    }
}
