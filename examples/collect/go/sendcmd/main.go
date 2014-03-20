package main

import (
	"fmt"
	"net"
	"os"

	"github.com/chenshuo/muduo-examples-in-go/muduo"
	"github.com/chenshuo/muduo-protorpc/examples/collect/go"
	"github.com/chenshuo/muduo-protorpc/go/muduorpc/rpc2"
)

func main() {
	conn, err := net.Dial("tcp", "localhost:12345")
	muduo.PanicOnError(err)
	stub := collect.NewCollectServiceClient(conn)
	req := new(rpc2.Empty)
	resp := new(collect.Result)
	empty := new(rpc2.Empty)
	switch os.Args[1] {
	case "f":
		stub.FlushFile(req, empty)
	case "n":
		stub.RollFile(req, resp)
	case "v":
		stub.Version(req, resp)
	case "q":
		stub.Quit(req, resp)
	case "r":
		stub.Restart(req, resp)
	}
	fmt.Println(resp)
	stub.Close()
}
