package com.chenshuo.muduo.rpc.sudoku;

import com.chenshuo.muduo.protorpc.sudoku.SudokuProto;
import com.chenshuo.muduo.rpc.NewChannelCallback;
import com.chenshuo.muduo.rpc.RpcChannel;
import com.chenshuo.muduo.rpc.RpcServer;

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
