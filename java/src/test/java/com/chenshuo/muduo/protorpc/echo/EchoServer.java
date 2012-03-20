package com.chenshuo.muduo.protorpc.echo;

import com.chenshuo.muduo.protorpc.RpcServer;
import com.chenshuo.muduo.protorpc.echo.EchoProto;
import com.chenshuo.muduo.protorpc.echo.EchoProto.EchoRequest;
import com.chenshuo.muduo.protorpc.echo.EchoProto.EchoResponse;
import com.chenshuo.muduo.protorpc.echo.EchoProto.EchoService.Interface;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcController;

public class EchoServer {

    public static void main(String[] args) {
        RpcServer server = new RpcServer();
        server.registerService(EchoProto.EchoService.newReflectiveService(new Interface() {
            @Override
            public void echo(RpcController controller, EchoRequest request, RpcCallback<EchoResponse> done) {
                done.run(EchoResponse.newBuilder().setPayload(request.getPayload()).build());
            }
        }));
        server.start(8888);
    }
}
