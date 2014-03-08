package muduorpc

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net"
	"net/rpc"

	"code.google.com/p/goprotobuf/proto"
)

type ServerCodec struct {
	conn    io.ReadWriteCloser
	r       io.Reader
	payload []byte
}

func (c *ServerCodec) ReadRequestHeader(r *rpc.Request) (err error) {
	if c.payload != nil {
		panic("payload is not nil")
	}

	var msg *RpcMessage
	msg, err = Decode(c.r)
	if err != nil {
		return
	}

	if *msg.Type != *MessageType_REQUEST.Enum() {
		err = fmt.Errorf("Wrong message type.")
		return
	}

	r.ServiceMethod = *msg.Service + "." + *msg.Method
	r.Seq = *msg.Id
	c.payload = msg.Request
	return nil
}

func (c *ServerCodec) ReadRequestBody(body interface{}) (err error) {
	if c.payload == nil {
		panic("payload is nil")
	}

	msg, ok := body.(proto.Message)
	if !ok {
		return fmt.Errorf("body is not a protobuf Message")
	}

	err = proto.Unmarshal(c.payload, msg)
	if err != nil {
		return
	}

	c.payload = nil
	return
}

func (c *ServerCodec) WriteResponse(r *rpc.Response, body interface{}) error {
	msg := new(RpcMessage)
	msg.Type = MessageType_RESPONSE.Enum()
	msg.Id = &r.Seq

	pb, ok := body.(proto.Message)
	if !ok {
		return fmt.Errorf("not a protobuf Message")
	}

	b, err := proto.Marshal(pb)
	if err != nil {
		return err
	}

	msg.Response = b

	return Send(c.conn, msg)
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
