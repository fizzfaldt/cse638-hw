#!/bin/bash

#$ -V
#$ -cwd
#$ -q normal
#$ -pe 12way 12			
#$ -N a
#$ -o output_a
#$ -e error_a
#$ -M rathakrishnanarun@gmail.com
#$ -l h_rt=01:30:00


export PATH=$PATH:$HOME/cilk/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HOME/cilk/lib
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/cage15-in.txt 0 0 0 0 4 64> serial/cage15-out.txt
