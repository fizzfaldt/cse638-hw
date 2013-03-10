#pragma once
#include <vector>
#include <istream>
#include <iostream>
#include <fstream>
#include <climits>
#include <pthread.h>

#ifdef __cilk
    #include <cilk.h>
#else
    #define cilk_spawn
    #define cilk_sync
    #define cilk_for for

//#define DISABLE_CILK
//#ifdef DISABLE_CILK
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

static const int INFINITY = INT_MAX;
static const int INVALID = INT_MAX;

class Queue {
    private:
        bool is_output;
        int head;
        int limit;
        std::vector<int> self_q;
        std::vector<int> &q;
        int original_name;
        int name;
        pthread_mutex_t lock;
    public:
        static const bool OUTPUT = true;
        static const bool INPUT = false;

    public:
        Queue();
        ~Queue();
        void init(int n, int name);
        void set_role(bool is_output);
        void reset(void);
        void enqueue(int value);
        int dequeue(void);
        void try_steal(Queue &victim, int min_steal_size);
        bool is_empty(void);
        void lock_for_stealing(void);
        bool try_lock_for_stealing(void);
        void unlock_for_stealing(void);
        int get_name(void);
};

class Graph {
    private:
        int n;
        int m;
        /* Adjacency list */
        std::vector< std::vector<int> > adj;
        std::vector<int> d;
        std::vector<int> owner;
        int p;
        std::vector<Queue> qs1;
        std::vector<Queue> qs2;
        std::vector<Queue> &qs_in;
        std::vector<Queue> &qs_out;
        int max_steal_attempts;
        int min_steal_size;
        bool opt_c;
        bool opt_g;
        bool opt_h;

    public:
        Graph(bool opt_c, bool opt_g, bool opt_h);
        void init(int max_steal_attempts, int min_steal_size,
                int n, int m, std::ifstream &ifs);
        unsigned long long computeChecksum(void);
        int serial_bfs(int s);
        int parallel_bfs(int s);
    private:
        void parallel_bfs_thread(int i);
        bool qs_are_empty(void);
        int find_max_d(void);
        int random_p(void);
};


class Problem {
    private:
        int n;
        int m;
        int r;
        Graph g;
        std::vector<int> sources;
    public:
        Problem(bool opt_c, bool opt_g, bool opt_h);
        void init(int max_steal_attempts, int min_steal_size, std::string filename);
        void run(bool parallel);
};
