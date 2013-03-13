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
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 8 32 > parallel/wikipedia-out1.txt 2>parallel/wiki-err1.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 8 64 > parallel/wikipedia-out2.txt 2>parallel/wiki-err2.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 8  128 > parallel/wikipedia-out3.txt 2>parallel/wiki-err3.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 8  196 > parallel/wikipedia-out4.txt 2>parallel/wiki-err4.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 16 32 > parallel/wikipedia-out5.txt 2>parallel/wiki-err5.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 16 64 > parallel/wikipedia-out6.txt 2>parallel/wiki-err6.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 16 128 > parallel/wikipedia-out7.txt 2>parallel/wiki-err7.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 16 196 > parallel/wikipedia-out8.txt 2>parallel/wiki-err8.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 24 32 > parallel/wikipedia-out9.txt 2>parallel/wiki-err9.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 24 64 > parallel/wikipedia-out10.txt 2>parallel/wiki-err10.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 24  128 > parallel/wikipedia-out11.txt 2>parallel/wiki-err11.txt
./a.out  /work/01905/rezaul/CSE638/HW1/turn-in/wikipedia-in.txt 0 0 0 0 24  196 > parallel/wikipedia-out12.txt 2>parallel/wiki-err12.txt
