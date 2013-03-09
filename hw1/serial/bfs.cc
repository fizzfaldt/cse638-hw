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
#include <stdlib.h>
#include <time.h>
#include "bfs.h"
using namespace std;


Queue::Queue(void) :
    is_output(false),
    head(0),
    limit(0),
    self_q(),
    q(self_q) {
}

void Queue::set_role(bool is_output) {
    this->is_output = is_output;
}

void Queue::init(int n, int name) {
    reset();
    q.resize(n);
    cilk_for(int u = 0; u < n; u++) {
        q[u] = INVALID;
    }
    this->original_name = this->name = name;
}

void Queue::reset(void) {
    head = limit = 0;
    q = self_q;
    name = original_name;
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

void Queue::try_steal(Queue &victim, int min_steal_size) {
    assert(!is_output);
    assert(!victim.is_output);

    //Don't steal if you have work.
    assert(head == limit);

    //Cache victim.head for both calculations.
    int vic_head = victim.head;
    if (victim.limit - vic_head >= min_steal_size) {
        //Steal half rounded down.
        head = (vic_head + 1 + victim.limit) / 2;
        limit = victim.limit;
        victim.limit = head;
        q = victim.q;
        name = victim.get_name();
    }
}

int Queue::get_name(void) {
    return name;
}

void Queue::lock_for_stealing(void) {
}

bool Queue::try_lock_for_stealing(void) {
    return true;
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
Graph<OPT_C, OPT_G, OPT_H>::Graph(void) :
    n(0),
    m(0),
    adj(),
    d(),
    owner(),
    p(__cilkrts_get_nworkers()),
    qs1(p, Queue()),
    qs2(p, Queue()),
    qs_in(qs1),
    qs_out(qs2) {
    srandom(time(NULL));
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Graph<OPT_C, OPT_G, OPT_H>::init(int max_steal_attempts, int min_steal_size,
        int n, int m, ifstream &ifs) {
    this->max_steal_attempts = max_steal_attempts;
    this->min_steal_size = min_steal_size;
    adj.clear();
    adj.resize(n);
    d.resize(n);
    owner.resize(n);

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
    for (int u = 0; u < n; ++u) {
        d[u] = INFINITY;
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
int Graph<OPT_C, OPT_G, OPT_H>::random_p(void) {
    int choices = 1;
    while (choices <= p) choices *= 2;
    while (true) {
        int r = random() & (choices - 1);
        if (r < p) {
            return r;
        }
    }
    //Low quality, faster:
    //return random() % p;
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
bool Graph<OPT_C, OPT_G, OPT_H>::qs_are_empty(Queue* queues) {
    cilk::reducer_opand< bool > empty;

    cilk_for (int i = 0; i < p; ++i) {
        empty &= queues[i].is_empty();
    }

    return empty.get_value();
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Graph<OPT_C, OPT_G, OPT_H>::parallel_bfs_thread(int i) {
    Queue &q_in = this->qs_in[i];
    Queue &q_out = this->qs_out[i];
    int name_out = q_out.get_name();
    do {
        int u;
        // Update each loop (changes when stealing).
        int name_in = q_in.get_name();
        while ((u = q_in.dequeue()) != INVALID) {
            if (!OPT_C || owner[u] == name_in) {
                FOR_EACH_GAMMA(v, adj[u]) {
                    if (d[*v] != INFINITY) {
                        d[*v] = d[u] + 1;
                        owner[*v] = name_out;
                        q_out.enqueue(*v);
                    }
                }
            }
        }
        q_in.lock_for_stealing();
        for (int t = 0; t < max_steal_attempts && q_in.is_empty(); ++t) {
            int r = random_p();
            if (qs_in[r].try_lock_for_stealing()) {
                q_in.try_steal(qs_in[r], min_steal_size);
                qs_in[r].unlock_for_stealing();
            }
        }
        q_in.unlock_for_stealing();
    } while (!q_in.is_empty());
}

template<bool OPT_C, bool OPT_G, bool OPT_H>
void Graph<OPT_C, OPT_G, OPT_H>::parallel_bfs(int s) {
    cilk_for(int u = 0; u < n; ++u) {
        d[u] = INFINITY;
        if (OPT_C) {
            owner[u] = INVALID;
        }
    }
    d[s] = 0;
    if (OPT_C) {
        owner[s] = 0;
    }

    cilk_for(int i = 0; i < p; ++i) {
        qs_in[i].init(n, i);
        qs_in[i].set_role(Queue::INPUT);
        qs_out[i].init(n, i);
        qs_out[i].set_role(Queue::OUTPUT);
    }

    qs_in[0].set_role(Queue::OUTPUT);
    qs_in[0].enqueue(s);
    qs_in[0].set_role(Queue::INPUT);

    while (!qs_are_empty(p, qs_in)) {
        if (OPT_H) {
            cilk_for (int i = 0; i < p; ++i) {
                cilk_spawn parallel_bfs_thread(i);
            }
        } else {
            for (int i = 1; i < p; ++i) {
                cilk_spawn parallel_bfs_thread(i);
            }
            cilk_spawn parallel_bfs_thread(0);
            cilk_sync;
        }
        swap(qs_in, qs_out);
        cilk_for(int i = 0; i < p; i++) {
            qs_in[i].set_role(Queue::INPUT);
            qs_out[i].set_role(Queue::OUTPUT);
            qs_out[i].reset();
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

long int random(void);
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
