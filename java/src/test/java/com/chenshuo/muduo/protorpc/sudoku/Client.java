package com.chenshuo.muduo.protorpc.sudoku;

import java.net.InetSocketAddress;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import com.chenshuo.muduo.protorpc.NewChannelCallback;
import com.chenshuo.muduo.protorpc.RpcChannel;
import com.chenshuo.muduo.protorpc.RpcClient;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto.SudokuRequest;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto.SudokuResponse;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto.SudokuService;
import com.google.protobuf.RpcCallback;

public class Client {

    private static void blockingConnect(InetSocketAddress addr) throws Exception {
        RpcClient client = new RpcClient();
        RpcChannel channel = client.blockingConnect(addr);
        //sendRequest(channel, client);
        SudokuService.BlockingInterface remoteService = SudokuProto.SudokuService.newBlockingStub(channel);
        SudokuRequest request = SudokuRequest.newBuilder().setCheckerboard("001010").build();
        SudokuResponse response = remoteService.solve(null, request);
        System.out.println(response);
        channel.disconnect();
        client.stop();
    }

    @SuppressWarnings("unused")
    private static void asyncConnect(InetSocketAddress addr) {
        final RpcClient client = new RpcClient();
        client.registerService(SudokuProto.SudokuService.newReflectiveService(new SudokuImpl()));
        client.startConnect(addr, new NewChannelCallback() {
            @Override
            public void run(RpcChannel channel) {
                sendAsyncRequest(channel, client);
            }
        });
    }

    private static void sendAsyncRequest(final RpcChannel channel, RpcClient client) {
        final CountDownLatch latch = new CountDownLatch(1);
        System.err.println("sendRequest " + channel);
        SudokuService remoteService = SudokuProto.SudokuService.newStub(channel);
        SudokuRequest request = SudokuRequest.newBuilder().setCheckerboard("001010").build();
        remoteService.solve(null, request, new RpcCallback<SudokuProto.SudokuResponse>() {
            @Override
            public void run(SudokuResponse parameter) {
                System.out.println(parameter);
                channel.disconnect();
                latch.countDown();
            }
        });
        try {
            latch.await(5, TimeUnit.SECONDS);
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        client.stop();
    }

    public static void main(String[] args) throws Exception {
        InetSocketAddress addr = new InetSocketAddress("localhost", 9981);
        // asyncConnect(addr);
        blockingConnect(addr);
    }
}
