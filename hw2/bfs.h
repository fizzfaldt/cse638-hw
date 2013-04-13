#pragma once
#include <vector>
#include <istream>
#include <iostream>
#include <fstream>
#include <climits>
#include <pthread.h>
#include <inttypes.h>

#define CXX_CHECKS 0
#define SINGLE_THREAD_PARANOID 0
#define CILK_VERIFY 0
#define DEBUG_PRINT 0
#define PARANOID 0
#define QUIT_EARLY 0

#ifndef __cilkplusplus
#include "fakecilk.h"
#endif

static const int INFINITY = INT_MAX;
static const int INVALID = INT_MAX;

class Graph;

class Graph {
    private:
        int n;
        int m;
        /* Adjacency list */
        std::vector< std::vector<int> > adj;
        std::vector<int> d;
        std::vector<int> owner;
        int p;
        std::vector<std::vector <int> > qs1;
        std::vector<std::vector <int> > qs2;
        std::vector<std::vector <int> > *qs_in;
        std::vector<std::vector <int> > *qs_out;
        int min_steal_size;

    public:
        Graph(void);
        void init(int min_steal_size,
                int n, int m, std::ifstream &ifs);
        unsigned long long computeChecksum(void);
        int serial_bfs(int s);
        int parallel_bfs(int s);
    private:
        void parallel_bfs_thread(int i);
        bool qs_are_empty(void);
	void parallel_forf(std::vector <int> const &, size_t, size_t);
        int find_max_d(void);
};


class GProblem {
    private:
        int n;
        int m;
        int r;
        Graph g;
        std::vector<int> sources;
    public:
        GProblem(int);
        void init(int min_steal_size, std::string filename);
        void run(bool parallel);
};

