#include <iostream>
#include <string>
#include <iterator>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <cstdio>
#include "pcon_component.h"


using namespace std;

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

	#ifdef _DEBUG_
		vector <pair <int, int> >::iterator e_i;
		for(e_i = edge.begin(); e_i != edge.end(); ++e_i){
			int u = (*e_i).first;	
			int v = (*e_i).second;	
			adj[u].push_back(v);
			adj[v].push_back(u);
		}
		dfs(0);
	#endif

	randomized_cc2(&vertex, &edge, &L);
	
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
