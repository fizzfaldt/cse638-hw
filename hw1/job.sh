#!/bin/bash

#$ -V
#$ -cwd
#$ -q development
#$ -pe 12way 12			
#$ -N a
#$ -o output_a
#$ -e error_a
#$ -M rathakrishnanarun@gmail.com
#$ -l h_rt=01:00:00


export PATH=$PATH:$HOME/cilk/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/cilk/lib
./a.out  test_graph.in 0 0 0 1 > serial_bfs_output
