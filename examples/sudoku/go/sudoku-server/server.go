package main

import (
	"log"
	"net"

	sudoku "github.com/chenshuo/muduo-protorpc/examples/sudoku/go"
	"github.com/chenshuo/muduo-protorpc/go/muduorpc"
)

type SudokuServer struct {
}

func (s *SudokuServer) Solve(req *sudoku.SudokuRequest, resp *sudoku.SudokuResponse) error {
	solved := true
	resp.Solved = &solved
	solution := "12345678"
	resp.Checkerboard = &solution
	return nil
}

func main() {
	service := new(SudokuServer)
	sudoku.RegisterSudokuService(service)
	l, err := net.Listen("tcp", ":9981")
	if err != nil {
		log.Fatal(err)
		return
	}
	muduorpc.Serve(l)
}
