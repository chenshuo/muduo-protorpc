package com.chenshuo.muduo.protorpc.sudoku;

import com.chenshuo.muduo.protorpc.sudoku.SudokuProto.SudokuRequest;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto.SudokuResponse;
import com.chenshuo.muduo.protorpc.sudoku.SudokuProto.SudokuService.Interface;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.RpcController;

public class SudokuImpl implements Interface {

    @Override
    public void solve(RpcController controller, SudokuRequest request,
            RpcCallback<SudokuResponse> done) {
        SudokuResponse resp = SudokuResponse.newBuilder()
                .setSolved(true).setCheckerboard("12345").build();
        done.run(resp);
    }
}
