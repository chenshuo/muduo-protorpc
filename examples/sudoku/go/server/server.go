package main

import (
	"log"
	"net"

	"github.com/chenshuo/muduo-protorpc/go/muduorpc"
	sudoku "github.com/chenshuo/muduo-protorpc/examples/sudoku/go"
)

type SudokuServer struct {
}

func (s *SudokuServer) Solve(req *sudoku.SudokuRequest, resp *sudoku.SudokuResponse) error {
	solved := false
	resp.Solved = &solved
	return nil
}

func main() {
	service := new(SudokuServer)
	sudoku.RegisterSudokuService(service)
	l, err := net.Listen("tcp", ":9981")
	if err != nil {
		log.Fatal(err)
		return;
	}
	muduorpc.Serve(l)
}
