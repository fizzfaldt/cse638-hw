/*
 * n m r
 * //m
 * //r (sources, repeat algo for this)
 * m lines: u v (sorted by u) <- directed edge
 * r lines sources: s (
 */
#include <assert.h>
#include <queue>
#include <utility>
#include "bfs.h"
using namespace std;


Queue::Queue(int size) :
    is_output(false),
    head(0),
    limit(0),
    self_q(size, INFINITY),
    q(self_q) {
}

void Queue::set_role(bool is_output) {
    this->is_output = is_output;
}

void Queue::reset(void) {
    head = limit = 0;
    q = self_q;
}

bool Queue::is_empty(void) {
    return head >= limit;
}

void Queue::enqueue(int value) {
    assert(is_output);
    assert(limit < (int)q.size());
    q.push_back(value);
    limit++;
}

int Queue::dequeue(void) {
    assert(!is_output);
    if (head < limit) {
        return q[head++];
    }
    return INFINITY;
}

void Queue::try_steal(Queue &victim) {
    assert(!is_output);
    assert(!victim.is_output);

    //Don't steal if you have work.
    assert(head == limit);

    //Steal half rounded down.
    head = (victim.head+1+victim.limit) / 2;
    limit = victim.limit;
    victim.limit = head;
    q = victim.q;
}


template<bool OPT_C, bool OPT_G, bool OPT_H>
Graph<OPT_C, OPT_G, OPT_H>::Graph(void) :
    n(0),
    m(0),
    adj(),
    d(),
    p(__cilkrts_get_nworkers())
{
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Graph<OPT_C, OPT_G, OPT_H>::init(int n, int m, ifstream &ifs) {
    adj.clear();
    adj.resize(n);
    d.resize(n);

    this->n = n;
    this->m = m;

    for (int i = 0; i < m; ++i) {
        int u, v;
        ifs >> u >> v;
        adj[u].push_back(v);
    }
}

#define FOR_EACH_GAMMA(v, vec) for (vector<int>::iterator v = (vec).begin(); v != (vec).end(); ++(v))


template<bool OPT_C, bool OPT_G, bool OPT_H>
unsigned long long Graph<OPT_C, OPT_G, OPT_H>::computeChecksum(void) {
    cilk::reducer_opadd< unsigned long long > chksum;

    cilk_for (int i = 0; i < n; ++i) {
        chksum += d[i];
    }

    return chksum.get_value();
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Graph<OPT_C, OPT_G, OPT_H>::serial_bfs(int s) {
    for (int i = 0; i < n; ++i) {
        d[i] = INFINITY;
    }
    d[s] = 0;
    queue<int> Q;
    Q.push(s);
    while (!Q.empty()) {
        int u = Q.front();
        Q.pop();

        FOR_EACH_GAMMA(v, adj[u]) {
            if (d[*v] != INFINITY) {
                d[*v] = d[u] + 1;
                Q.push(*v);
            }
        }
    }
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
bool Graph<OPT_C, OPT_G, OPT_H>::qs_are_empty(int p, Queue* Qs) {
    cilk::reducer_opand< bool > empty;

    cilk_for (int i = 0; i < p; ++i) {
        empty &= Qs[i].is_empty();
    }

    return empty.get_value();
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Graph<OPT_C, OPT_G, OPT_H>::parallel_bfs(int s) {
    cilk_for(int u = 0; u < n; ++u) {
        d[u] = INFINITY;
    }
    d[s] = 0;
    Queue* Q[2][p];
    Queue* Qin = &Q[0];
    Queue* Qout = &Q[1];
    cilk_for(int i = 0; i < p; ++i) {
        Qin[i] = new Queue(n);
        Qin[i].set_role(Queue::INPUT);
        Qout[i] = new Queue(n);
        Qout[i].set_role(Queue::OUTPUT);
    }

    Qin[0].set_role(Queue::OUTPUT);
    Qin[0].enqueue(s);
    Qin[0].set_role(Queue::INPUT);

    while (!qs_are_empty(p, Qin)) {
        if (OPT_H) {
            cilk_for (int i = 0; i < p; ++i) {
                cilk_spawn parallel_bfs_thread(i, Qin, Qout);
            }
        } else {
            for (int i = 1; i < p; ++i) {
                cilk_spawn parallel_bfs_thread(i, Qin, Qout);
            }
            cilk_spawn parallel_bfs_thread(0, Qin, Qout);
            cilk_sync;
        }
        swap(Qin, Qout);
        cilk_for(int i = 0; i < p; i++) {
            Qin[i].set_role(Queue::INPUT);
            Qout[i].set_role(Queue::OUTPUT);
            Qout[i].reset();
        }
    }
}

//Maybe original
//Maybe with C[i] array (c)
//Maybe (g)
//Maybe (h)
//DEFINITELY f-c
//DEFINITELY g-c


template<bool OPT_C, bool OPT_G, bool OPT_H>
Problem<OPT_C, OPT_G, OPT_H>::Problem(void) :
    n(0),
    m(0),
    r(0),
    g(),
    sources() {
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Problem<OPT_C, OPT_G, OPT_H>::init(string filename) {
    ifstream ifs(filename.c_str());
    ifs >> n >> m >> r;
    g.init(n, m, ifs);
    for (int i = 0; i < r; ++i) {
        int s;
        ifs >> s;
        sources.push_back(s);
    }
    ifs.close();
}

int main(void) {
    return 0;
}
