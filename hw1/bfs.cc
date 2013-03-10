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
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "bfs.h"
using namespace std;


Queue::Queue(void) :
    is_output(false),
    head(0),
    limit(0),
    self_q(),
    q(self_q) {
    int r = pthread_mutex_init(&lock, NULL);
    assert(r==0);
}

Queue::~Queue(void) {
    int r = pthread_mutex_destroy(&lock);
    assert(r==0);
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
    int r = pthread_mutex_lock(&lock);
    assert(r==0);
}

void Queue::unlock_for_stealing(void) {
    int r = pthread_mutex_unlock(&lock);
    assert(r==0);
}

bool Queue::try_lock_for_stealing(void) {
    int r = pthread_mutex_unlock(&lock);
    if (r == EBUSY) {
        return false;
    }
    assert(r==0);
    return true;
}

Graph::Graph(bool opt_c, bool opt_g, bool opt_h) :
    n(0),
    m(0),
    adj(),
    d(),
    owner(),
    p(__cilkrts_get_nworkers()),
    qs1(p, Queue()),
    qs2(p, Queue()),
    qs_in(qs1),
    qs_out(qs2),
    opt_c(opt_c),
    opt_g(opt_g),
    opt_h(opt_h) {
    srandom(time(NULL));
}

void Graph::init(int max_steal_attempts, int min_steal_size,
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

unsigned long long Graph::computeChecksum(void) {
    cilk::reducer_opadd<unsigned long long> chksum;

    cilk_for (int i = 0; i < n; ++i) {
        chksum += d[i] == INFINITY ? n : d[i];
    }

    return chksum.get_value();
}

int Graph::serial_bfs(int s) {
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
    int maxd = 0;
    for (int u = 0; u < n; ++u) {
        maxd = max(maxd, d[u]);
    }
    if (maxd == INFINITY) {
        maxd = n;
    }
    return maxd;
}

int Graph::random_p(void) {
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

bool Graph::qs_are_empty(void) {
    cilk::reducer_opand< bool > empty;

    cilk_for (int i = 0; i < p; ++i) {
        empty &= qs_in[i].is_empty();
    }

    return empty.get_value();
}

void Graph::parallel_bfs_thread(int i) {
    Queue &q_in = this->qs_in[i];
    Queue &q_out = this->qs_out[i];
    int name_out = q_out.get_name();
    do {
        int u;
        // Update each loop (changes when stealing).
        int name_in = q_in.get_name();
        while ((u = q_in.dequeue()) != INVALID) {
            if (!opt_c || owner[u] == name_in) {
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

int Graph::parallel_bfs(int s) {
    cilk_for(int u = 0; u < n; ++u) {
        d[u] = INFINITY;
        if (opt_c) {
            owner[u] = INVALID;
        }
    }
    d[s] = 0;
    if (opt_c) {
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

    while (!qs_are_empty()) {
        if (opt_h) {
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

    //TODO: Use max reducer here.
    int maxd = 0;
    for (int u = 0; u < n; ++u) {
        maxd = max(maxd, d[u]);
    }
    if (maxd == INFINITY) {
        maxd = n;
    }
    return maxd;
}

//Maybe original
//Maybe with C[i] array (c)
//Maybe (g)
//Maybe (h)
//DEFINITELY f-c
//DEFINITELY g-c

Problem::Problem(bool opt_c, bool opt_g, bool opt_h) :
    n(0),
    m(0),
    r(0),
    g(opt_c, opt_g, opt_h),
    sources() {
}

void Problem::init(
        int max_steal_attempts, int min_steal_size, string filename) {
    ifstream ifs(filename.c_str());
    ifs >> n >> m >> r;
    g.init(max_steal_attempts, min_steal_size, n, m, ifs);
    for (int i = 0; i < r; ++i) {
        int s;
        ifs >> s;
        sources.push_back(s);
    }
    ifs.close();
}

void Problem::run(bool parallel) {
    for (int s = 0; s < r; ++s) {
        int maxd;
        if (parallel) {
            maxd = g.parallel_bfs(sources[s]);
        } else {
            maxd = g.serial_bfs(sources[s]);
        }
        printf("%d %lld\n", maxd, g.computeChecksum());
    }
}

int main(int argc, const char *argv[]) {
    bool opt_c = random() & 0x1;
    bool opt_g = random() & 0x1;;
    bool opt_h = random() & 0x1;;
    bool do_parallel = random() & 0x1;
    int max_steal_attempts = random() & 0xFFFF;
    int min_steal_size = random() & 0xFF;

    Problem p(opt_c, opt_g, opt_h);
    p.init(max_steal_attempts, min_steal_size, "filename");
    p.run(do_parallel);

    argc = argc;
    argv = argv;
//    p.init(filename);
    return 0;
}

