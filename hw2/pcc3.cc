#include <iostream>
#include <string>
#include <iterator>
#include <cmath>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include "pcon_component.h"
#define PRINT1 0
#define IMP_DEBUG 0
#define CC3_DEBUG 0

nmap U;
nmap PhD;
const double alpha = pow(15.0/16.0,0.5);

using namespace std;

edge_set  select_random(edge_set* edge, int md){
	int size = edge->limit;
	edge_set select_edge;
	if(md == size){
		select_edge.limit = edge->limit;
		int *uset = (int *)malloc(sizeof(int) * edge->limit);
		int *vset = (int *)malloc(sizeof(int) * edge->limit);
		#if CC3_DEBUG
		if(!vset || !uset){
			cout<<"Heck that"<<endl;
		}
		#endif
		cilk_for(int i = 0; i < edge->limit; ++i){
			uset[i] = edge->uset[i];
			vset[i] = edge->vset[i];
		}
		set_edge(&select_edge, uset, vset, size);
		return select_edge;
	}
	set <pair <int, int> > pos;
	set <pair <int, int> > :: iterator i_pos;

	int *o_uset = edge->uset;
	int *o_vset = edge->vset;

	#if CC3_DEBUG
		cout<<"Edge Selection Started "<<pos.size()<<endl;
	#endif

	while((int)pos.size() < md){
		int ind = random(size);

		pos.insert(make_pair(o_uset[ind],o_vset[ind]));
	}

	int * uset = (int *) malloc(sizeof(int) * pos.size());
	int * vset = (int *) malloc(sizeof(int) * pos.size());

	int ind = 0;

	for(i_pos = pos.begin(); i_pos != pos.end(); ++i_pos){
		uset[ind] = (*i_pos).first;
		vset[ind] = (*i_pos).second;
		++ind;
		assert(ind <= (int) pos.size());
	}
	#if CC3_DEBUG
		cout<<"Edge Set formed"<<endl;
	#endif
	set_edge(&select_edge, uset, vset, pos.size());
	return select_edge;
}

void randomized_cc3(vertex_set * vertex,
					edge_set * edge,
					nmap *L,
					nmap *PhD, int d, int d_max){
	//TODO Check if passing reference speeds thing up. Otherwise send copy
	//Not PhD size and L size are not equal to vertex size

	#if PRINT1
		cout<<"LEVEL "<<d<<" ";
		cout<<"VERTEX SIZE "<<vertex->limit<<" EDGE SIZE "<<edge->limit<<endl;
	#endif
	int * lset = L->set;

	if(d <= d_max){
		int md = ceil(edge->limit * pow(alpha,d));
		edge_set selected_edge = select_random(edge, md);
		#if CC3_DEBUG
			cout<<"DONE"<<endl;
		#endif

		cilk_for(int v_i = 0; v_i < vertex->limit; ++v_i){
			int u = vertex->set[v_i];
			assert(lset[u] == u);
			U.set[u] = false;
		}

		cilk_for(int si = 0; si < selected_edge.limit; ++si){
			int u = selected_edge.uset[si];
			int v = selected_edge.vset[si];
			int ru = lset[u];
			int rv = lset[v];

			if(ru != rv && PhD->set[ru] && PhD->set[rv] && ru == lset[ru] && rv == lset[rv]){
				//assert(L[ru] == ru && L[rv] == rv);
				//assert(find(all(vertex),rv) != vertex.end());
				U.set[ru] = U.set[rv] = true;
				N.set[ru] = rv;
				N.set[rv] = ru;
			}
		}

		free(selected_edge.uset);
		free(selected_edge.vset);

		#if CC3_DEBUG
			cout<<"DONE"<<endl;
		#endif

		vertex_set pvertex;

		int * input = (int *) malloc(sizeof(int) *vertex->limit);
		for(int v_i = 0; v_i < vertex->limit; ++v_i){
			int u = vertex->set[v_i];
			if(!U.set[u]){
				PhD->set[u] = 0;
				input[v_i] = 0;
			}
			else
				input[v_i] = 1;
		}
		
		pvertex = select_subset(vertex->set, input, vertex->limit);

		random_hook(&pvertex, L, &N);

		free(pvertex.set);

		vertex_set root_vertex = select_root_vertex(vertex, L);
		//set PhD of all to false
		//set PhD of vertices in root_vertex that have U[v] ==1 as PhD
		#if PRINT1
			cout<<"LEVEL "<<d<<" ";
			cout<<"VERTEX SIZE "<<vertex->limit<<" EDGE SIZE "<<edge->limit<<endl;
			for(int i = 0; i < edge->limit; ++i){
				cout<<edge->uset[i]<<" "<<edge->vset[i]<<endl;
			}
		#endif

		randomized_cc3(&root_vertex, edge, L, PhD, d+1, d_max);

		free(root_vertex.set);

		for(int v_i = 0; v_i < vertex->limit; ++v_i){
			int u = vertex->set[v_i];
			int index = lset[u];
			lset[u] = lset[index];
		}
	}
	if(d == 0){
		//vector <pair <int, int> > root_edge = select_root_edge_cc3(edge, L);
		edge_set root_edge = select_root_edge(edge, L);
		vertex_set root_vertex = select_root_vertex(vertex, L);

		#if IMP_DEBUG
		cout<<"CC2 Here"<<endl;
		cout<<"EDGE SIZE "<<root_edge.limit<<" VERTEX SIZE "<<root_vertex.limit<<endl;
		cout<<"VERTEX"<<endl;
		for(int i = 0; i < root_vertex.limit; ++i)
			cout<<root_vertex.set[i]+1<<" "<<lset[root_vertex.set[i]]+1<<endl;
		cout<<"EDGE"<<endl;
		for(int i = 0; i <root_edge.limit; i++){
			cout<<root_edge.uset[i]+1<<" "<<root_edge.vset[i]+1<<endl;
		}
		#endif

		randomized_cc2(&root_vertex, &root_edge, L);
		for(int v_i = 0; v_i < vertex->limit; ++v_i){
			int u = vertex->set[v_i];
			lset[u] = lset[lset[u]];
		}
		free(root_edge.uset);
		free(root_edge.vset);
		free(root_vertex.set);
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

	#if 0
	cout<<"ALPHA "<<alpha;
	#endif
	cin>>n>>m;
	vector <int> comp_cnt;
	vertex_set vertex;
	edge_set edge;
	nmap L;

	int * set = (int *) malloc(sizeof(int) * n);
	int * uset = (int *) malloc(sizeof(int) * m);
	int * vset = (int *) malloc(sizeof(int) * m);

	set_vertex(&vertex, set, n);
	set_edge(&edge, uset, vset, m);
	set_nmap(&L, n);
	set_nmap(&N, n);
	set_nmap(&hook, n);
	set_nmap(&coin, n);
	set_nmap(&PhD, n);
	set_nmap(&U, n);

	for(int i = 0; i < n; ++i){
		set[i] = i;
		L.set[i] = i;	
		comp_cnt.push_back(0);
		PhD.set[i]=1;
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
	int dmax = ceil(0.25*log(n)/log(1.0/alpha));
	cout<<dmax<<endl;
	randomized_cc3(&vertex, &edge, &L, &PhD, 0, dmax);
	
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

	int pos = (int) comp_cnt.size();
	cilk_for(int i = 0; i < pos; ++i){
		if(comp_cnt[i] == 0){
			if(i-1 >= 0){
				if(comp_cnt[i-1] > 0){
					pos = i;
				}
			}
		}
	}
	//int size = binary_search(comp_cnt, 0, comp_cnt.size(), 0);
	cout<<pos<<endl;

	for(int i = 0; i < pos; i++){
		cout<<comp_cnt[i]<<endl;
	}

	free(vertex.set);
	free(edge.uset);
	free(edge.vset);
	free(L.set);
	free(N.set);
	free(hook.set);
	free(coin.set);
	free(PhD.set);
	free(U.set);
	error = clock_gettime(CLOCK_REALTIME, &end_t);
	assert(error==0);

	double start_d = start_t.tv_sec + start_t.tv_nsec / 1e9;
	double end_d = end_t.tv_sec + end_t.tv_nsec / 1e9;

	printf("Time taken: %f\n", end_d - start_d);

}
