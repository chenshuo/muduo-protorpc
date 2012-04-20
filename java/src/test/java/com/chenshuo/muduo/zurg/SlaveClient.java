package com.chenshuo.muduo.zurg;

import java.net.InetSocketAddress;

import com.chenshuo.muduo.protorpc.RpcChannel;
import com.chenshuo.muduo.protorpc.RpcClient;
import com.chenshuo.muduo.zurg.proto.SlaveProto;
import com.chenshuo.muduo.zurg.proto.SlaveProto.GetFileContentRequest;
import com.chenshuo.muduo.zurg.proto.SlaveProto.GetFileContentResponse;
import com.chenshuo.muduo.zurg.proto.SlaveProto.SlaveService;
import com.google.protobuf.ServiceException;

public class SlaveClient {

    public static void main(String[] args) throws Exception {
        InetSocketAddress addr = new InetSocketAddress(args[0], Integer.parseInt(args[1]));

        String fileName = "/proc/self/stat";
        getFileContent(addr, fileName);
    }

    private static void getFileContent(InetSocketAddress addr, String fileName) throws ServiceException {
        RpcClient client = new RpcClient();
        RpcChannel channel = client.blockingConnect(addr);
        SlaveService.BlockingInterface slaveService = SlaveProto.SlaveService.newBlockingStub(channel);
        GetFileContentRequest request = GetFileContentRequest.newBuilder().setFileName(fileName).build();
        GetFileContentResponse response = slaveService.getFileContent(null, request);
        System.out.println(response);
        System.out.println(response.getContent());
        channel.disconnect();
        client.stop();
    }

}
