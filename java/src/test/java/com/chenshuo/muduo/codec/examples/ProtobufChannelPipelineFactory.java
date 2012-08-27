package com.chenshuo.muduo.codec.examples;

import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.ChannelPipelineFactory;
import org.jboss.netty.channel.Channels;
import org.jboss.netty.channel.SimpleChannelHandler;
import org.jboss.netty.handler.codec.frame.LengthFieldBasedFrameDecoder;
import org.jboss.netty.handler.codec.frame.LengthFieldPrepender;

import com.chenshuo.muduo.codec.ProtobufDecoder;
import com.chenshuo.muduo.codec.ProtobufEncoder;
import com.google.protobuf.Message;

public class ProtobufChannelPipelineFactory implements ChannelPipelineFactory {

    private ProtobufDecoder decoder = new ProtobufDecoder();
    private ProtobufEncoder encoder = new ProtobufEncoder();
    private LengthFieldPrepender frameEncoder = new LengthFieldPrepender(4);
    private Class<? extends SimpleChannelHandler> handler;

    public ProtobufChannelPipelineFactory(Class<? extends SimpleChannelHandler> handler) {
        this.handler = handler;
    }

    public void addMessageType(Message message) {
        decoder.addMessageType(message);
    }

    @Override
    public ChannelPipeline getPipeline() throws Exception {
        ChannelPipeline p = Channels.pipeline();
        p.addLast("frameDecoder", new LengthFieldBasedFrameDecoder(16 * 1024 * 1024, 0, 4, 0, 4));
        p.addLast("protobufDecoder", decoder);

        p.addLast("frameEncoder", frameEncoder);
        p.addLast("protobufEncoder", encoder);

        p.addLast("handler", handler.newInstance());
        return p;
    }
}
