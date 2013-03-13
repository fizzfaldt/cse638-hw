#!/bin/bash

#$ -V
#$ -cwd
#$ -q normal
#$ -pe 12way 12			
#$ -N a
#$ -o output_a
#$ -e error_a
#$ -M rathakrishnanarun@gmail.com
#$ -l h_rt=01:00:00


export PATH=$PATH:$HOME/cilk/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/cilk/lib
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/rmat100M-in.txt 0 0 0 1 > parallel/rmat100M-out.txt 2>parallel/rmat100M-err.txt
