/*
 * n m r
 * //m
 * //r (sources, repeat algo for this)
 * m lines: u v (sorted by u) <- directed edge
 * r lines sources: s (
 */
#include <assert.h>
#include <queue>
#include "bfs.h"
using namespace std;


Queue::Queue(int size) :
    is_output(false),
    head(0),
    limit(0),
    self_q(size, Graph::INFINITY),
    q(self_q) {
}

void Queue::set_role(bool is_output) {
    this->is_output = is_output;
}

void Queue::reset(void) {
    head = limit = 0;
    q = self_q;
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
    return Graph::INFINITY;
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

Graph::Graph(void) :
    n(0),
    m(0),
    adj(),
    d() {
}

void Graph::init(int n, int m, ifstream &ifs) {
    adj.clear();
    adj.resize(n);
    d.resize(n);

    this->n = n;
    this->m = m;

    for (int i = 0; i < m; i++) {
        int u, v;
        ifs >> u >> v;
        adj[u].push_back(v);
    }
}

#define FOR_EACH_GAMMA(v, vec) for (vector<int>::iterator v = (vec).begin(); v != (vec).end(); ++(v))


unsigned long long Graph::computeChecksum(void) {
    cilk::reducer_opadd< unsigned long long > chksum;

    cilk_for (int i = 0; i < n; i++) {
        chksum += d[i];
    }

    return chksum.get_value();
}

void Graph::serial_bfs(int s) {
    for (int i = 0; i < n; i++) {
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

Problem::Problem(void) :
    n(0),
    m(0),
    r(0),
    g(),
    sources() {
}

void Problem::init(string filename) {
    ifstream ifs(filename.c_str());
    ifs >> n >> m >> r;
    g.init(n, m, ifs);
    for (int i = 0; i < r; i++) {
        int s;
        ifs >> s;
        sources.push_back(s);
    }
    ifs.close();
}

int main(void) {
    return 0;
}
