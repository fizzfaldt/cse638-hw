#pragma once
#include <vector>
#include <istream>
#include <iostream>
#include <fstream>

#define DISABLE_CILK
#ifdef DISABLE_CILK
#define cilk_for for
#define cilk_spawn
#define cilk_sync
inline int __cilkrts_get_nworkers(void) {
    return 1;
}

namespace cilk {
    template<class T>
    class reducer_opand {
        private:
            T value;
        public:
            reducer_opand(void) {
                value = true;
            }

            reducer_opand & operator&=(const reducer_opand &rhs) {
                value &= rhs.value;
                return *this;
            }

            reducer_opand & operator&=(const T &rhs) {
                value &= rhs;
                return *this;
            }

            T get_value() {
                return value;
            }

    };

    template<class T>
    class reducer_opadd {
        private:
            T value;
        public:
            reducer_opadd(void) {
                value = 0;
            }

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

static const int INFINITY = -1;

class Queue {
    private:
        bool is_output;
        int head;
        int limit;
        std::vector<int> self_q;
        std::vector<int> &q;
    public:
        static const bool OUTPUT = true;
        static const bool INPUT = false;

    public:
        Queue(int size);
        void set_role(bool is_output);
        void reset(void);
        void enqueue(int value);
        int dequeue(void);
        void try_steal(Queue &victim);
        bool is_empty(void);
};

template<bool OPT_C, bool OPT_G, bool OPT_H>
class Graph {
    private:
        int n;
        int m;
        /* Adjacency list */
        std::vector< std::vector<int> > adj;
        std::vector<int> d;
        int p;

    public:
        Graph(void);
        void init(int n, int m, std::ifstream &ifs);
        unsigned long long computeChecksum(void);
        void serial_bfs(int s);
        void parallel_bfs(int s);
    private:
        void parallel_bfs_thread(int i, int Q_o);
        bool qs_are_empty(int p, Queue* Qs);
};


template<bool OPT_C = false, bool OPT_G = false, bool OPT_H = false>
class Problem {
    private:
        int n;
        int m;
        int r;
        Graph<OPT_C, OPT_G, OPT_H> g;
        std::vector<int> sources;
    public:
        Problem(void);
        void init(std::string filename);
};
