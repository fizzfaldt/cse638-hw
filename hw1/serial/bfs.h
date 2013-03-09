#pragma once
#include <vector>
#include <istream>
#include <iostream>
#include <fstream>

#define DISABLE_CILK
#ifdef DISABLE_CILK
#define cilk_for for
namespace cilk {
    template<class T>
    class reducer_opadd {
        private:
            T value;
        public:
            reducer_opadd & operator+=(const reducer_opadd &rhs) {
                value += rhs.value;
                return *this;
            }

            reducer_opadd & operator+=(const T &rhs) {
                value += rhs;
                return *this;
            }

            T get_value() {
                return value;
            }

    };
}
#endif

class Graph {
    private:
        int n;
        int m;
        /* Adjacency list */
        std::vector< std::vector<int> > adj;
        std::vector<int> d;
    public:
        static const int INFINITY = -1;
        Graph(void);
        void init(int n, int m, std::ifstream &ifs);
        unsigned long long computeChecksum(void);
        void serial_bfs(int s);
};

class Queue {
    private:
        bool is_output;
        int head;
        int limit;
        std::vector<int> self_q;
        std::vector<int> &q;

    public:
        Queue(int size);
        void set_role(bool is_output);
        void reset(void);
        void enqueue(int value);
        int dequeue(void);
        void try_steal(Queue &victim);
};

class Problem {
    private:
        int n;
        int m;
        int r;
        Graph g;
        std::vector<int> sources;
    public:
        Problem(void);
        void init(std::string filename);
};
