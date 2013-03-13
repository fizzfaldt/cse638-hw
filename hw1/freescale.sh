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
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/freescale-in.txt 0 0 0 0 > freescale-out.txt
