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

#ifdef __cilkplusplus
#include <cilk.h>
#include <reducer_opadd.h>
#include <reducer_opand.h>
#include <reducer_max.h>
//#include <cilk_mutex.h>
#endif

#include "bfs.h"

using namespace std;
cilk::context ctx;


Graph::Graph(void):
    n(0),
    m(0),
    adj(),
    d(),
    owner(),
    p(ctx.get_worker_count()),
    qs1(p, vector<int>()),
    qs2(p, vector<int>()),
    #if 0
    qs1(p, Queue()),
    qs2(p, Queue()),
    #endif
    qs_in(&qs1),
    qs_out(&qs2) {
//    cilk::context ctx;
//    p = ctx->get_worker_count();
    srandom(time(NULL));
}

void Graph::init(int min_steal_size,
        int n, int m, ifstream &ifs) {
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
        assert(u>0);
        assert(v>0);
        adj[u-1].push_back(v-1);
    }
}

#define FOR_EACH_GAMMA(v, vec) for (vector<int>::iterator v = (vec).begin(); v != (vec).end(); ++(v))

unsigned long long Graph::computeChecksum(void) {
    cilk::reducer_opadd<unsigned long long> chksum;

    cilk_for (int i = 0; i < n; ++i) {
        chksum += d[i] == INFINITY ? n : d[i];
    }
#if CILK_VERIFY 
    int mind = INT_MAX;
    int maxd = INT_MIN;
    unsigned long long chksum_ser = 0;
    for (int i = 0; i < n; ++i) {
        assert(d[i] >= 0);
        int change = d[i] == INFINITY ? n : d[i];
        chksum_ser += change;
        mind = std::min(mind, change);
        maxd = std::max(maxd, change);
    }
    fprintf(stderr, "min=%d, max=%d\n", mind, maxd);
    fflush(stderr);

    assert(chksum.get_value() == chksum_ser);
#endif

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
    return maxd;
}

bool Graph::qs_are_empty(void) {
    cilk::reducer_opand< bool > empty(true);

    cilk_for (int i = 0; i < p; ++i) {
        empty &= (*qs_in)[i].empty();
    }
#if CILK_VERIFY 
    bool empty_ser = true;
    for (int i = 0; i < p; ++i) {
        bool before = empty_ser;
        empty_ser &= (*qs_in)[i].is_empty();
        assert(empty_ser == (before && (*qs_in)[i].is_empty()));
    }
    assert(empty.get_value() == empty_ser);
#endif

    return empty.get_value();
}

void Graph::parallel_forf(vector<int> const & q, size_t start, size_t limit){
	size_t size = limit - start;
	if((int) size <= this->min_steal_size){
		//TODO fix qout
		//get worker id
		int worker_id = cilk::current_worker_id();
		assert(worker_id >= 0 && worker_id < p);
		vector<int> &q_out = (*qs_out)[worker_id];
		for(size_t j = start; j < limit; ++j){
			int u = q[j];
			FOR_EACH_GAMMA(v, adj[u]) {
				if (d[*v] == INFINITY) {
					d[*v] = d[u] + 1;
					if (adj[*v].size() > 0) {
						q_out.push_back(*v);
					}
				}
			}
		}
		return;
	}
	size_t mid = (start + limit)/2;
	cilk_spawn parallel_forf(q, start, mid);
	parallel_forf(q, mid, limit);
	cilk_sync;

}

void Graph::parallel_bfs_thread(int i) {
    vector<int> &q_in = (*qs_in)[i];
    parallel_forf(q_in, 0, q_in.size());
}

int Graph::parallel_bfs(int s) {
    cilk_for(int u = 0; u < n; ++u) {
        d[u] = INFINITY;
    }
#if DEBUG_PRINT 
    fprintf(stderr, "setting d[%d] source = 0. n=%d\n", s, n);
    fflush(stderr);
#endif
    
    d[s] = 0;

    cilk_for(int i = 0; i < p; ++i) {
	(*qs_in)[i].clear();
	(*qs_in)[i].reserve(n);
	(*qs_out)[i].clear();
	(*qs_out)[i].reserve(n);
	#if 0
        (*qs_in)[i].init(n, i, this);
        (*qs_in)[i].set_role(Queue::INPUT);
        (*qs_out)[i].init(n, i, this);
        (*qs_out)[i].set_role(Queue::OUTPUT);
	#endif
    }

    (*qs_in)[0].push_back(s);
    #if 0
    (*qs_in)[0].set_role(Queue::OUTPUT);
    (*qs_in)[0].enqueue(s);
    (*qs_in)[0].set_role(Queue::INPUT);
    #endif
#if PARANOID 
    assert(!qs_are_empty());
#endif

#if DEBUG_PRINT 
    int source_levels = 0;
#endif
    while (!qs_are_empty()) {
#if DEBUG_PRINT 
        fprintf(stderr, "Source level= %d\n", source_levels);
        fflush(stderr);
        source_levels++;
#endif
    for (int i = 0; i < p-1; ++i) {
	cilk_spawn parallel_bfs_thread(i);
    }
    parallel_bfs_thread(p-1);
    cilk_sync;
#if CXX_CHECKS
        intptr_t a = (intptr_t)qs_in;
        intptr_t b = (intptr_t)qs_out;
#endif
        //Note:  std::swap here sometimes causes unexpected behavior
        //because of optimization.  Swap manually.
        //std::vector<Queue> *temp;
        std::vector<std::vector <int> > *temp;
        temp = qs_in;
        qs_in = qs_out;
        qs_out = temp;

#if CXX_CHECKS
        assert((intptr_t)qs_in == b);
        assert((intptr_t)qs_out == a);
#endif
        cilk_for(int i = 0; i < p; i++) {
		#if 0
            (*qs_in)[i].set_role(Queue::INPUT);
            (*qs_out)[i].set_role(Queue::OUTPUT);
            (*qs_out)[i].reset();
	    #endif
		(*qs_out)[i].clear();
        }
    }


    cilk::reducer_max<int> maxd(0);
    cilk_for (int u = 0; u < n; ++u) {
    	if(d[u] == INFINITY) continue;
        maxd = cilk::max_of(maxd, d[u]);
    }
#if CILK_VERIFY
    int maxd_ser = 0;
    for (int u = 0; u < n; ++u) {
    	if(d[u] == INFINITY) continue;
        maxd_ser = max(maxd_ser, d[u]);
    }
    assert(maxd_ser == maxd.get_value());
#endif
    return maxd.get_value();
}

//Maybe original
//Maybe with C[i] array (c)
//Maybe (g)
//Maybe (h)
//DEFINITELY f-c
//DEFINITELY g-c

GProblem::GProblem(int a):
    n(0),
    m(0),
    r(0),
    g(),
    sources() {
    	a=a;
}

int ceil_lg(int n) {
    assert(n>0);
    int64_t x = 1;
    int p = 0;
    while (x < n) {
        p++;
        x *= 2;
    }
    return max(p, 1);
}

void GProblem::init(
        int min_steal_size, string filename) {
    ifstream ifs(filename.c_str());
    ifs >> n >> m >> r;


    g.init(min_steal_size, n, m, ifs);
    for (int i = 0; i < r; ++i) {
        int s;
        ifs >> s;
        assert(s>0);
        sources.push_back(s-1);
    }
    ifs.close();
}

void GProblem::run(bool parallel) {
    for (int s = 0; s < r; ++s) {
        int maxd;
        if (parallel) {
            maxd = g.parallel_bfs(sources[s]);
        } else {
            maxd = g.serial_bfs(sources[s]);
        }
        printf("%d %llu\n", maxd, g.computeChecksum());
#if QUIT_EARLY 
        fprintf(stderr, "QUITTING\n");
        break;
#endif
    }
}

int cilk_main(int argc, char *argv[]) {
    assert(argc == 6);
    struct timespec start_t;
    struct timespec end_t;
    int error;
    error = clock_gettime(CLOCK_REALTIME, &start_t);
    assert(error==0);

    string file_name(argv[1]);
    bool do_parallel = atoi(argv[5]);

    int min_steal_size = 64;

    GProblem p(2);
    p.init(min_steal_size, file_name);
    p.run(do_parallel);

    error = clock_gettime(CLOCK_REALTIME, &end_t);
    assert(error==0);

    double start_d = start_t.tv_sec + start_t.tv_nsec / 1e9;
    double end_d = end_t.tv_sec + end_t.tv_nsec / 1e9;

    fprintf(stderr, "Time taken: %f\n", end_d - start_d);
//    p.init(filename);
    return 0;
}

