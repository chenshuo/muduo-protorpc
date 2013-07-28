package main

import (
	"log"
	"net"

	echo "github.com/chenshuo/muduo-protorpc/examples/rpcbench2/go"
	"github.com/chenshuo/muduo-protorpc/go/muduorpc"
)

type EchoServer struct {
}

func (s *EchoServer) Echo(req *echo.EchoRequest, resp *echo.EchoResponse) error {
	resp.Payload = req.Payload
	return nil
}

func main() {
	service := new(EchoServer)
	echo.RegisterEchoService(service)
	l, err := net.Listen("tcp", ":8888")
	if err != nil {
		log.Fatal(err)
		return
	}
	muduorpc.Serve(l)
}
