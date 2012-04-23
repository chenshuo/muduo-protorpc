package com.chenshuo.muduo.zurg;

import java.net.InetSocketAddress;
import java.util.Arrays;

import com.chenshuo.muduo.protorpc.RpcChannel;
import com.chenshuo.muduo.protorpc.RpcClient;
import com.chenshuo.muduo.zurg.proto.SlaveProto.GetFileContentRequest;
import com.chenshuo.muduo.zurg.proto.SlaveProto.GetFileContentResponse;
import com.chenshuo.muduo.zurg.proto.SlaveProto.RunCommandRequest;
import com.chenshuo.muduo.zurg.proto.SlaveProto.RunCommandResponse;
import com.chenshuo.muduo.zurg.proto.SlaveProto.SlaveService;
import com.google.protobuf.ByteString;

public class SlaveClient {

    private RpcClient client;
    private RpcChannel channel;
    private SlaveService.BlockingInterface slaveService;

    public SlaveClient(InetSocketAddress addr) {
        client = new RpcClient();
        channel = client.blockingConnect(addr);
        slaveService = SlaveService.newBlockingStub(channel);
    }

    public void close() {
        channel.disconnect();
        client.stop();
    }

    public void getFileContent(String fileName) throws Exception {
        GetFileContentRequest request = GetFileContentRequest.newBuilder().setFileName(fileName).build();
        GetFileContentResponse response = slaveService.getFileContent(null, request);
        System.out.println(response.getErrorCode());
        System.out.println(response.getFileSize());
        ByteString content = response.getContent();
        System.out.println(content.size());
        if (content.size() < 8192) {
            System.out.println(content.toStringUtf8());
        }
    }

    public void runCommand(String cmd, String... args) throws Exception {
        RunCommandRequest request = RunCommandRequest.newBuilder()
                .setCommand(cmd)
                .addAllArgs(Arrays.asList(args))
                .setMaxStdout(1048576)
                .setMaxStderr(65536)
                .setTimeout(5)
                .setCwd(".")
                .build();
        RunCommandResponse response = slaveService.runCommand(null, request);
        System.out.println("\nerr = " + response.getErrorCode());
        if (response.getErrorCode() == 0) {
            System.out.println("pid " + response.getPid());
            System.out.println("seconds " + (response.getFinishTimeUs() - response.getStartTimeUs()) / 1000000.0);
            System.out.println("exit " + response.getExitStatus());
            System.out.println("signaled " + response.getSignaled());
            System.out.print(response.getCoredump() ? "core dump\n" : "");
            System.out.println("max mem " + response.getMemoryMaxrssKb());
            System.out.println("stdout len " + response.getStdOutput().size());
            if (response.getStdOutput().size() > 0 && response.getStdOutput().size() < 8192) {
                System.out.println(response.getStdOutput().toStringUtf8());
            }
            System.out.println("stderr len " + response.getStdError().size());
            if (response.getStdError().size() > 0 && response.getStdError().size() < 8192) {
                System.out.println(response.getStdError().toStringUtf8());
            }
        }
    }

    public static void main(String[] args) throws Exception {
        InetSocketAddress addr = new InetSocketAddress(args[0], Integer.parseInt(args[1]));
        SlaveClient slaveClient = new SlaveClient(addr);

        slaveClient.getFileContent("/proc/uptime");
        slaveClient.runCommand("/bin/NotExist");
        slaveClient.runCommand("/etc/hosts");
        slaveClient.runCommand("/bin/pwd");
        slaveClient.runCommand("echo", "$PWD");
        slaveClient.runCommand("sort", "/etc/services");
        slaveClient.runCommand("ls", "/proc/self/fd", "-l");
        slaveClient.runCommand("ls", "/notexist", "-l");
        slaveClient.runCommand("cat", "/dev/zero");
        slaveClient.runCommand("bin/dummy_load", "1");
        slaveClient.runCommand("bin/dummy_load", "-n");
        slaveClient.runCommand("bin/dummy_load", "-c");
        slaveClient.runCommand("bin/dummy_load", "-a");
        slaveClient.runCommand("bin/dummy_load", "-p");
        slaveClient.runCommand("bin/dummy_load", "-v", "a", "b");
        slaveClient.runCommand("bin/dummy_load", "-m", "102400");
        slaveClient.runCommand("bin/dummy_load", "-o", "100");
        slaveClient.runCommand("bin/dummy_load", "-e", "100");
        slaveClient.runCommand("bin/dummy_load", "-s", "1.5");
        slaveClient.runCommand("bin/dummy_load", "-o", "100000");
        slaveClient.runCommand("bin/dummy_load", "-o", "10000000");
        slaveClient.runCommand("bin/dummy_load", "-e", "100000");
        slaveClient.runCommand("bin/dummy_load", "-s", "20");
        slaveClient.close();
    }
}
