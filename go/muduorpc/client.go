package muduorpc

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net/rpc"
	"strings"

	"code.google.com/p/goprotobuf/proto"
)

type ClientCodec struct {
	conn io.ReadWriteCloser
	r    io.Reader
}

func (c *ClientCodec) WriteRequest(r *rpc.Request, body interface{}) error {
	msg := new(RpcMessage)
	msg.Type = MessageType_REQUEST.Enum()
	msg.Id = &r.Seq
	last_dot := strings.LastIndex(r.ServiceMethod, ".")
	if last_dot < 0 {
		panic(fmt.Sprintf("Invalid ServiceMethod '%s'", r.ServiceMethod))
	}
	service := r.ServiceMethod[:last_dot]
	msg.Service = &service
	method := r.ServiceMethod[last_dot+1:]
	msg.Method = &method

	pb, ok := body.(proto.Message)
	if !ok {
		return fmt.Errorf("not a protobuf Message")
	}

	b, err := proto.Marshal(pb)
	if err != nil {
		return err
	}

	msg.Request = b

	wire, err := Encode(msg)
	if err != nil {
		return err
	}

	n, err := c.conn.Write(wire)
	if err != nil {
		return err
	} else if n != len(wire) {
		return fmt.Errorf("Incomplete write %d of %d bytes", n, len(wire))
	}
	return nil
}

func (c *ClientCodec) ReadResponseHeader(r *rpc.Response) error {
	log.Fatal("ReadResponseHeader")
	return nil
}

func (c *ClientCodec) ReadResponseBody(body interface{}) error {
	return nil
}

func (c *ClientCodec) Close() error {
	return c.conn.Close()
}

func NewClientCodec(conn io.ReadWriteCloser) *ClientCodec {
	codec := new(ClientCodec)
	codec.conn = conn
	codec.r = bufio.NewReader(conn)
	return codec
}
