package main

import (
	"fmt"
	nqueens "github.com/chenshuo/muduo-protorpc/examples/nqueens/go"
	"log"
	"net"
	"os"
	"strconv"
)

var threshold = 14

type Shard struct {
	first_row, second_row int32
	count                 int64
}

func solve(solver nqueens.NQueensService, N, first_row int32, second_row *int32) int64 {
	req := new(nqueens.SubProblemRequest)
	resp := new(nqueens.SubProblemResponse)
	req.Nqueens = &N
	req.FirstRow = &first_row
	req.SecondRow = second_row
	err := solver.Solve(req, resp)
	if err == nil {
		log.Print(req, resp)
		return *resp.Count
	} else {
		log.Print(err)
	}
	return -1
}

func mapper(solver nqueens.NQueensService, N int) (result []Shard) {
	shards := 0
	ch := make(chan Shard)
	for i := 0; i < (N+1)/2; i++ {
		if N <= threshold {
			shards++
			go func(first_row int32) {
				count := solve(solver, int32(N), first_row, nil)
				ch <- Shard{first_row, -1, count}
			}(int32(i))
		} else {
			for j := 0; j < N; j++ {
				shards++
				go func(first_row, second_row int32) {
					count := solve(solver, int32(N), first_row, &second_row)
					ch <- Shard{first_row, second_row, count}
				}(int32(i), int32(j))
			}
		}
	}
	for i := 0; i < shards; i++ {
		result = append(result, <-ch)
	}
	return
}

func verify(N int, shards []Shard) {
	results := map[int32][]int32{}
	for _, shard := range shards {
		x := results[shard.first_row]
		results[shard.first_row] = append(x, shard.second_row)
	}

	// log.Print(results)
	// TODO: verify shards
}

func reducer(N int, shards []Shard) int64 {
	results := map[int]int64{}

	for _, shard := range shards {
		results[int(shard.first_row)] += shard.count
	}
	log.Print("results = ", results)

	var total int64
	for i := 0; i < (N+1)/2; i++ {
		if N%2 == 1 && i == N/2 {
			total += results[i]
		} else {
			total += 2 * results[i]
		}
	}
	return total
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: nqueens-client N")
		return
	}

	N, _ := strconv.Atoi(os.Args[1])
	conn, err := net.Dial("tcp", "localhost:9352")
	if err != nil {
		log.Fatal(err)
	}

	solver := nqueens.NewNQueensServiceClient(conn)
	log.Print("N = ", N)
	shards := mapper(solver, N)
	log.Print("shards = ", shards)
	verify(N, shards)
	solution := reducer(N, shards)
	log.Print("solution = ", solution)

	solver.Close()
}
