package muduorpc

import (
	"encoding/binary"
	"fmt"
	"hash/adler32"
	"io"

	"code.google.com/p/goprotobuf/proto"
)

// FIXME: find better way of writing this
func encode(msg *RpcMessage) ([]byte, error) {
	payload, err := proto.Marshal(msg)
	if err != nil {
		return nil, err
	}

	wire := make([]byte, 12+len(payload))

	// size
	binary.BigEndian.PutUint32(wire, uint32(8+len(payload)))

	// marker
	if copy(wire[4:], "RPC0") != 4 {
		panic("What the hell")
	}

	// payload
	if copy(wire[8:], payload) != len(payload) {
		panic("What the hell")
	}

	// checksum
	checksum := adler32.Checksum(wire[4 : 8+len(payload)])
	binary.BigEndian.PutUint32(wire[8+len(payload):], checksum)

	return wire, nil
}

func Send(c io.Writer, msg *RpcMessage) error {
	wire, err := encode(msg)
	if err != nil {
		return err
	}

	n, err := c.Write(wire)
	if err != nil {
		return err
	} else if n != len(wire) {
		return fmt.Errorf("Incomplete write %d of %d bytes", n, len(wire))
	}

	return nil
}

func Decode(r io.Reader) (msg *RpcMessage, err error) {
	msg = new(RpcMessage)
	err = DecodeGeneral(r, msg, "RPC0")
	return msg, err
}

func DecodeGeneral(r io.Reader, msg proto.Message, tag string) (err error) {
	header := make([]byte, 4)
	_, err = io.ReadFull(r, header)
	if err != nil {
		return
	}

	length := binary.BigEndian.Uint32(header)
	if length > 64*1024*1024 || length < uint32(len(tag)+4) {
		return fmt.Errorf("Invalid length %d", length)
	}

	payload := make([]byte, length)
	_, err = io.ReadFull(r, payload)
	if err != nil {
		return
	}

	if string(payload[:len(tag)]) != tag {
		return fmt.Errorf("Wrong marker")
	}

	checksum := adler32.Checksum(payload[:length-4])
	if checksum != binary.BigEndian.Uint32(payload[length-4:]) {
		return fmt.Errorf("Wrong checksum")
	}

	return proto.Unmarshal(payload[4:length-4], msg)
}
