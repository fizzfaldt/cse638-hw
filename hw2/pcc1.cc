#include <iostream>
#include <string>
#include <iterator>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <cstdio>
#include <cilk.h>
#include "pcon_component.h"
#define cilk_for1 for
using namespace std;


void randomized_cc1(int n, edge_set *edge, nmap * L){
	cout<<"EDGE "<<edge->limit<<endl;
	if(edge->limit == 0) return;
	//cout<<"Coin "<<endl;
	cilk_for(int i = 0; i < n; ++i){
		int val = random(100);
		coin.set[i] = val/50;
	//	cout<<coin.set[i]<<" ";
	}
	//cout<<endl;*
	set <int> edgeSet;
	cilk_for1(int ei = 0; ei < edge->limit; ++ei){
		int u = edge->uset[ei];
		int v = edge->vset[ei];
		assert(u == L->set[u] && v == L->set[v]);
		edgeSet.insert(u);
		edgeSet.insert(v);
	}
	cout<<"EDGE SET"<<endl;
	for(set <int> :: iterator ei = edgeSet.begin(); ei != edgeSet.end(); ++ei){
		cout<<*ei<<endl;
	}
	set <int> hitSet;
	cilk_for1(int ei = 0; ei < edge->limit; ++ei){
		int u = edge->uset[ei];
		int v = edge->vset[ei];
		if(coin.set[u] == TAIL && coin.set[v] == HEAD){
			hitSet.insert(u);
			L->set[u] = L->set[v];
		}
		if(coin.set[v] == TAIL && coin.set[u] == HEAD){
			hitSet.insert(v);
			L->set[v] = L->set[u];
		}
	}
	cout<<"Atleast "<<hitSet.size()<<" / "<<edge->limit<<endl;
	edge_set root = select_root_edge(edge, L);
	randomized_cc1(n, &root, L);
	free(root.uset);
	free(root.vset);
	cilk_for(int ei = 0; ei < edge->limit; ++ei){
		int u = edge->uset[ei];
		int v = edge->vset[ei];
		if(L->set[u] == v){
			L->set[u] = L->set[v];
		}
		if(L->set[v] == u){
			L->set[v] = L->set[u];
		}
	}
}


int cilk_main(){
	srand(time(NULL));
	int n,m;
	struct timespec start_t;
	struct timespec end_t;
	int error;
	error = clock_gettime(CLOCK_REALTIME, &start_t);
	assert(error==0);
	cin>>n>>m;
	vector <int> comp_cnt;
	vertex_set vertex;
	nmap L;
	edge_set  edge;

	int * set = (int *) malloc(sizeof(int) * n);

	int * uset = (int *) malloc(sizeof(int) * m);
	int * vset = (int *) malloc(sizeof(int) * m);

	set_vertex(&vertex, set, n);
	set_edge(&edge, uset, vset, m);
	set_nmap(&L, n);
	set_nmap(&hook, n);
	set_nmap(&coin, n);
	set_nmap(&N,n);

	for(int i = 0; i < n; ++i){
		set[i] = i;
		L.set[i] = i;	
		comp_cnt.push_back(0);
	}

	for(int i = 0; i < m; ++i){
		int a, b;
		cin>>a>>b;
		a--;b--;
		uset[i] = a;
		vset[i] = b;
	}

	randomized_cc1(n, &edge, &L);
	
	#ifdef _DEBUG_
	for(int i = 0; i < n; ++i){
		cout<<i+1<<" "<<L.set[i]+1<<endl;
	}
	#endif	

	for(int i = 0; i < n; ++i){
		comp_cnt[L.set[i]]++;
	}

	#ifdef _DEBUG_
		for(int i = 0; i < n; ++i){
			if(comp_cnt[i] == 1){
				cout<<"SINGLE "<<endl;
				dfs(i);
			}
		}
	#endif

	sort(all(comp_cnt), std::greater<int>());

	int pos = comp_cnt.size();
	//TODO parallelize this
	for(int i = 0; i < (int)comp_cnt.size(); ++i){
		if(comp_cnt[i] == 0){
			if(i-1 >= 0){
				if(comp_cnt[i-1] > 0){
					pos = i;
					break;
				}
			}
			else {
				pos = i;
				break;
			}
		}
	}
	//int size = binary_search(comp_cnt, 0, comp_cnt.size(), 0);
	cout<<pos<<endl;

	for(int i = 0; i < pos; i++){
		cout<<comp_cnt[i]<<endl;
	}
	free(coin.set);
	free(L.set);
	free(N.set);
	free(hook.set);
	free(set);
	free(uset);
	free(vset);
	error = clock_gettime(CLOCK_REALTIME, &end_t);
	assert(error==0);

	double start_d = start_t.tv_sec + start_t.tv_nsec / 1e9;
	double end_d = end_t.tv_sec + end_t.tv_nsec / 1e9;

	printf("Time taken: %f\n", end_d - start_d);
}
