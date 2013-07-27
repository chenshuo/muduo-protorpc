package muduorpc

import (
	"code.google.com/p/goprotobuf/proto"
	"encoding/binary"
	"hash/adler32"
)

// FIXME: find better way of writing this
func Encode(msg *RpcMessage) ([]byte, error) {
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
