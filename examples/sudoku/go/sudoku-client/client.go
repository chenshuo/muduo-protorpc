package main

import (
	"log"
	"net"

	sudoku "github.com/chenshuo/muduo-protorpc/examples/sudoku/go"
)

func main() {
	conn, err := net.Dial("tcp", "localhost:9981")
	if err != nil {
		log.Fatal(err)
	}

	client := sudoku.NewSudokuServiceClient(conn)
	req := new(sudoku.SudokuRequest)
	resp := new(sudoku.SudokuResponse)
	puzzle := "001010"
	req.Checkerboard = &puzzle
	err = client.Solve(req, resp)
	if err == nil {
		log.Print(resp.String())
	}

	client.Close()
}
