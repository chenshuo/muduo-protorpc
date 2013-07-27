package muduorpc

import (
	"bufio"
	"code.google.com/p/goprotobuf/proto"
	"encoding/binary"
	"fmt"
	"io"
	"log"
	"net"
	"net/rpc"
)

type ServerCodec struct {
	conn io.ReadWriteCloser
	r    io.Reader
	reqb  []byte
}

func (c *ServerCodec) ReadRequestHeader(req *rpc.Request) (err error) {
	if c.reqb != nil {
		panic("reqb is not nil")
	}
	header := make([]byte, 4)
	_, err = io.ReadFull(c.r, header)
	if err != nil {
		return
	}

	length := binary.BigEndian.Uint32(header)
	payload := make([]byte, length)
	_, err = io.ReadFull(c.r, payload)
	if err != nil {
		return
	}

	// FIXME: check "RPC0" and checksum

	msg := new(RpcMessage)
	err = proto.Unmarshal(payload[4 : length-4], msg)
	if err != nil {
		return
	}

	req.ServiceMethod = *msg.Service + "." + *msg.Method
	req.Seq = *msg.Id
	c.reqb = msg.Request
	return nil
}

func (c *ServerCodec) ReadRequestBody(req interface{}) error {
	if c.reqb == nil {
		panic("reqb is nil")
	}

	msg, ok := req.(proto.Message)
	if !ok {
		return fmt.Errorf("req is not a protobuf Message")
	}

	err := proto.Unmarshal(c.reqb, msg)
	if err != nil {
		return err
	}

	c.reqb = nil
	return nil
}

func (c *ServerCodec) WriteResponse(*rpc.Response, interface{}) error {
	log.Fatal("WriteResponse")
	return nil
}

func (c *ServerCodec) Close() error {
	return c.conn.Close()
}

func NewServerCodec(conn io.ReadWriteCloser) *ServerCodec {
	codec := new(ServerCodec)
	codec.conn = conn
	codec.r = bufio.NewReader(conn)
	return codec
}

func ServeConn(conn io.ReadWriteCloser) {
	rpc.ServeCodec(NewServerCodec(conn))
}

func Serve(l net.Listener) {
	defer l.Close()
	for {
		conn, err := l.Accept()
		if err != nil {
			log.Fatal(err)
		} else {
			go ServeConn(conn)
		}
	}
}
