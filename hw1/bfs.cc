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
#include <cilk.h>
#include <reducer_opadd.h>
#include <reducer_opand.h>
#include <cilk_mutex.h>
#include "bfs.h"
#define cilkrts_get_nworkers 12
using namespace std;


Queue::Queue(void) :
    is_output(false),
    head_pair(0),
    limit_pair(0),
    self_q(),
    q(self_q),
    self_edge_prefix_sum_exclusive(),
    edge_prefix_sum_exclusive(self_edge_prefix_sum_exclusive) {
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

void Queue::init(int n, int name, Graph *g, bool opt_g) {
    this->g = g;
    reset();
    q.resize(n);
    edge_prefix_sum_exclusive.resize(n+1); // May need to store one element after end.
    cilk_for(int u = 0; u < n; u++) {
        q[u] = INVALID;
    }
    this->original_name = this->name = name;
    // this->adj = adj;
    this->opt_g = opt_g;
}

void Queue::reset(void) {
    head_pair = limit_pair = 0;
    q = self_q;
    edge_prefix_sum_exclusive = self_edge_prefix_sum_exclusive;
    name = original_name;
    edge_steal_attempted = false;
}

bool Queue::is_empty(void) {
    return head_pair >= limit_pair;
}

void Queue::enqueue(int value) {
    assert(is_output);

    int limit = get_limit();
    assert(limit < (int)q.size());
    q.push_back(value);
    if (opt_g) {
        set_limit(limit + 1);
    } else {
        set_limit(limit + 1);
    }
}

int Queue::dequeue(void) {
    assert(!is_output);
    assert(!opt_g);
    int head = get_head();
    int limit = get_limit();
    if (head < limit) {
        set_head(head+1);
        return q[head];
    }
    return INFINITY;
}

void Queue::dequeue(int *node, int *edge) {
    assert(!is_output);
    assert(opt_g);

    int head_node, head_edge;
    int limit_node, limit_edge;
    get_head(&head_node, &head_edge);
    get_limit(&limit_node, &limit_edge);

    int candidate_node;
    // limit_node is 1 higher than the last node we actually touch.
    // limit_edge is 1 higher than the last edge we actually touch IN
    //                             the last node we actually touch
    if (head_node < limit_node) { // At least one node to look at.
        candidate_node = q[head_node];
        if (g->weight(candidate_node, name) == head_edge) {  // Last edge in node
            // Done with this node.
            head_node++;
            head_edge = 0;
            candidate_node = q[head_node];
        }
        assert(g->weight(candidate_node, name) > 0);
        if (head_node < limit_node-1  // This isn't the last node.
                || head_edge < limit_edge ) {  // This is the last node, but not last edge
            *node = head_node;
            *edge = head_edge;
            set_head(head_node, head_edge+1);
            return;
        }
    }
    // Set "easy" nothing left condition.
    head_pair = limit_pair;
    *node = INFINITY;
    *edge = INFINITY;
}

int Queue::get_head(void) {
    return (int)(head_pair >> 32);
}

int Queue::get_limit(void) {
    return (int)(limit_pair >> 32);
}

void Queue::set_head(int head) {
    assert(!opt_g);  //otherwise use 2-arg function.
    assert((head_pair & 0xFFFFFFFFLL) == 0);  // No edge info
    assert(head >= 0);
    head_pair = static_cast<uint64_t>(head) << 32;
}

void Queue::set_limit(int limit) {
    assert(!opt_g);  //otherwise use 2-arg function.
    assert((limit_pair & 0xFFFFFFFFLL) == 0);  // No edge info
    assert(limit >= 0);
    limit_pair = static_cast<uint64_t>(limit) << 32;
}

void Queue::get_head(int *node, int *edge) {
    assert(opt_g);  //otherwise use 1-arg function.
    uint64_t copied = head_pair;
    *node = static_cast<int>(copied >> 32);
    *edge = static_cast<int>(copied & 0xFFFFFFFFLL);
    assert(*node >= 0);
    assert(*edge >= 0);
}

void Queue::get_limit(int *node, int *edge) {
    assert(opt_g);  //otherwise use 1-arg function.
    uint64_t copied = limit_pair;
    *node = static_cast<int>(copied >> 32);
    *edge = static_cast<int>(copied & 0xFFFFFFFFLL);
    assert(*node >= 0);
    assert(*edge >= 0);
}

void Queue::set_head(int node, int edge) {
    assert(opt_g);  //otherwise use 1-arg function.
    assert(node >= 0);
    assert(edge >= 0);
    head_pair = static_cast<uint64_t>(node) << 32;
    head_pair += edge;
}

void Queue::set_limit(int node, int edge) {
    assert(opt_g);  //otherwise use 1-arg function.
    assert(node >= 0);
    assert(edge >= 0);
    limit_pair = static_cast<uint64_t>(node) << 32;
    limit_pair += edge;
}

void Queue::try_steal(Queue &victim, int min_steal_size) {
    assert(!is_output);
    assert(!victim.is_output);
    assert(!opt_g);
    assert(min_steal_size >= 2); //Otherwise what does stealing half mean?

    //Don't steal if you have work.
    assert(this->head_pair == this->limit_pair);

    int vic_head = victim.get_head();
    int vic_limit = victim.get_limit();
    if (vic_limit - vic_head >= min_steal_size) {
        //Steal half rounded down.
        this->head_pair = this->limit_pair = 0;
        int stolen_head = (vic_head + 1 + vic_limit) / 2;
        int stolen_limit = vic_limit;
        set_head(stolen_head);
        set_limit(stolen_limit);
        victim.set_limit(stolen_head);
        q = victim.q;
        name = victim.get_name();
    }
}

//TODO: Use this
void Queue::try_steal_edges(Queue &victim, int min_steal_size) {
    assert(!is_output);
    assert(!victim.is_output);
    assert(opt_g);
    assert(min_steal_size >= 2); //Otherwise what does stealing half mean?

    //Don't steal if you have work.
    assert(this->head_pair >= this->limit_pair);

    int vic_head_node, vic_head_edge;
    int vic_limit_node, vic_limit_edge;
    victim.get_head(&vic_head_node, &vic_head_edge);
    victim.get_limit(&vic_limit_node, &vic_limit_edge);
    if (vic_limit_node - vic_head_node >= min_steal_size) {
        //We can steal half of the NODES
        //Steal half rounded down.
        this->head_pair = this->limit_pair = 0;
        int stolen_head_node = (vic_head_node + 1 + vic_limit_node) / 2;
        int stolen_head_edge = 0;
        int stolen_limit_node = vic_limit_node;
        int stolen_limit_edge = vic_limit_edge;
        victim.set_limit(stolen_head_node, g->weight(victim.q[stolen_head_node-1], victim.name));
        this->set_head(stolen_head_node, stolen_head_edge);
        this->set_limit(stolen_limit_node, stolen_limit_edge);
        q = victim.q;
        name = victim.get_name();
        edge_steal_attempted = false;
        return;
    }
    //Try to steal half of the EDGES
    if (!victim.edge_steal_attempted) {
        // Initial edge steal information.
        victim.edge_steal_attempted = true;
        int64_t sum = 0;
        for (int i = vic_head_node; i < vic_limit_node; i++) {
            victim.edge_prefix_sum_exclusive[i] = sum;
            sum += g->weight(victim.q[i], victim.name);
        }
        // victim.q[vic_limit_node] might not exist (off end).
        victim.edge_prefix_sum_exclusive[vic_limit_node] = sum;
    }
    int64_t total_edges = victim.edge_prefix_sum_exclusive[vic_limit_node];
    total_edges -= victim.edge_prefix_sum_exclusive[vic_head_node];
    total_edges -= vic_head_edge;
    total_edges += vic_limit_edge;
    total_edges -= g->weight(victim.q[vic_limit_node-1], victim.name);

    // total_edges is the remaining edges to work on.
    if (total_edges >= min_steal_size) {
        // Try to steal half (rounded down) of the remaining edges.
        int64_t edge_find = (total_edges + 1) / 2;

        for (int i = vic_head_node; i < vic_limit_node; i++) {
            int edges_here = g->weight(victim.q[i], victim.name);
            int offset = 0;

            if (i == vic_head_node) {
                edges_here -= vic_head_edge;
                offset = vic_head_edge;
            }
            if (i == vic_limit_node - 1) {
                edges_here += vic_limit_edge;
                edges_here -= g->weight(victim.q[i], victim.name);
            }
            if (edges_here >= edge_find) {
                // Found it.

                this->head_pair = this->limit_pair = 0;
                int stolen_head_node = i;
                int stolen_head_edge = edge_find + offset;
                int stolen_limit_node = vic_limit_node;
                int stolen_limit_edge = vic_limit_edge;
                victim.set_limit(stolen_head_node + 1, edge_find + offset);
                this->set_head(stolen_head_node, stolen_head_edge);
                this->set_limit(stolen_limit_node, stolen_limit_edge);
                q = victim.q;
                q = victim.q;
                edge_prefix_sum_exclusive = victim.edge_prefix_sum_exclusive;
                name = victim.get_name();
                edge_steal_attempted = true;
                return;
            }
        }
        assert(false);
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
    //p(cilkrts_get_nworkers()),
    p(12),
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
            if (d[*v] == INFINITY) {
                d[*v] = d[u] + 1;
                Q.push(*v);
            }
        }
    }
    int maxd = 0;
    for (int u = 0; u < n; ++u) {
    	if(d[u] == INFINITY) continue;
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

int Graph::weight(int u, int name) {
    if (opt_c && owner[u] != name) {
        return 1;
    }
    return adj[u].size();
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
                    if (d[*v] == INFINITY) {
                        d[*v] = d[u] + 1;
                        owner[*v] = name_out;
                        if (adj[*v].size() > 0) {
                            q_out.enqueue(*v);
                        }
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
        qs_in[i].init(n, i, this, opt_g);
        qs_in[i].set_role(Queue::INPUT);
        qs_out[i].init(n, i, this, opt_g);
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
    	if(d[u] == INFINITY) continue;
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

int cilk_main(int argc, char *argv[]) {
    time_t start_t;
    time(&start_t);
    string file_name(argv[1]);
    bool opt_c = atoi(argv[2]);
    bool opt_g = atoi(argv[3]);
    bool opt_h = atoi(argv[4]);
    bool do_parallel = atoi(argv[5]);

    int max_steal_attempts = random() & 0xFFFF;
    int min_steal_size = random() & 0xFF;

    Problem p(opt_c, opt_g, opt_h);
    p.init(max_steal_attempts, min_steal_size, file_name);
    p.run(do_parallel);

    time_t stop_t;
    time(&stop_t);

    double diff = difftime(stop_t, start_t);
    argc = argc;
    argv = argv;
    printf( "Time taken: %f\n",diff);
//    p.init(filename);
    return 0;
}

