package com.chenshuo.muduo.protorpc.sudoku;

import com.chenshuo.muduo.protorpc.NewChannelCallback;
import com.chenshuo.muduo.protorpc.RpcChannel;
import com.chenshuo.muduo.protorpc.RpcServer;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto;

public class Server {

    public static void main(String[] args) {
        RpcServer server = new RpcServer();
        server.registerService(SudokuProto.SudokuService.newReflectiveService(new SudokuImpl()));
        server.setNewChannelCallback(new NewChannelCallback() {

            @Override
            public void run(RpcChannel channel) {
                // TODO call client

            }
        });
        server.start(9981);
    }
}
