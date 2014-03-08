package muduorpc

import (
	"bufio"
	"fmt"
	"io"
	"net/rpc"
	"strings"

	"code.google.com/p/goprotobuf/proto"
)

type ClientCodec struct {
	conn    io.ReadWriteCloser
	r       io.Reader
	payload []byte
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

	return Send(c.conn, msg)
}

func (c *ClientCodec) ReadResponseHeader(r *rpc.Response) (err error) {
	if c.payload != nil {
		panic("payload is not nil")
	}

	var msg *RpcMessage
	msg, err = Decode(c.r)
	if err != nil {
		return
	}

	if *msg.Type != *MessageType_RESPONSE.Enum() {
		err = fmt.Errorf("Wrong message type.")
		return
	}

	r.Seq = *msg.Id
	c.payload = msg.Response
	return nil
}

// FIXME: merge dup code with ServerCodec.ReadRequestBody()
func (c *ClientCodec) ReadResponseBody(body interface{}) (err error) {
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

func (c *ClientCodec) Close() error {
	return c.conn.Close()
}

func NewClientCodec(conn io.ReadWriteCloser) *ClientCodec {
	codec := new(ClientCodec)
	codec.conn = conn
	codec.r = bufio.NewReader(conn)
	return codec
}
