package com.chenshuo.muduo.codec.examples;

import java.net.InetSocketAddress;
import java.util.concurrent.Executors;

import org.jboss.netty.bootstrap.ServerBootstrap;
import org.jboss.netty.channel.ChannelFactory;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.MessageEvent;
import org.jboss.netty.channel.SimpleChannelHandler;
import org.jboss.netty.channel.socket.nio.NioServerSocketChannelFactory;

import com.chenshuo.muduo.codec.QueryProtos.Query;
import com.google.protobuf.Message;

public class Server {

    public static class ServerHandler extends SimpleChannelHandler {
        @Override
        public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) throws Exception {
            System.out.println(e.getMessage().getClass().getName());
            if (e.getMessage() instanceof Message) {
                System.out.println(e.getMessage());
            }
        }
    }

    public static void main(String[] args) {
        ChannelFactory channelFactory = new NioServerSocketChannelFactory(
                Executors.newCachedThreadPool(),
                Executors.newCachedThreadPool());
        ServerBootstrap bootstrap = new ServerBootstrap(channelFactory);
        ProtobufChannelPipelineFactory pipelineFactory = new ProtobufChannelPipelineFactory(ServerHandler.class);
        pipelineFactory.addMessageType(Query.getDefaultInstance());
        bootstrap.setPipelineFactory(pipelineFactory);
        int port = Integer.parseInt(args[0]);
        bootstrap.bind(new InetSocketAddress(port));
    }
}
