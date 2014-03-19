package main

import (
	"compress/gzip"
	"fmt"
	"os"

	"github.com/chenshuo/muduo-protorpc/examples/collect/go"
	"github.com/chenshuo/muduo-protorpc/go/muduorpc"
)

func main() {
	var last, lastsys int64
	for _, filename := range os.Args[1:] {
		f, err := os.Open(filename)
		if err != nil {
			panic(err)
		}
		defer f.Close()
		// bufio?
		gf, err := gzip.NewReader(f)
		if err != nil {
			panic(err)
		}
		defer gf.Close()
		total := 0
		for {
			msg := new(collect.SystemInfo)
			err = muduorpc.DecodeGeneral(gf, msg, "SYS0")
			if err != nil {
				break
			}
			time := float64(*msg.MuduoTimestamp) / 1e6
			cpu := *msg.AllCpu.UserMs
			syscpu := *msg.AllCpu.SysMs
			usage := cpu - last
			sysusage := syscpu - lastsys
			if last != 0 {
				fmt.Printf("%.2f %.2f %.2f\n", time, float64(usage)/1000.0, float64(sysusage)/1000.0)
			}
			last = cpu
			lastsys = syscpu
			total++
		}
		println(total)
	}
}
