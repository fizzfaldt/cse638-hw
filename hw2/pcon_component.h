#include <vector>
#include <algorithm>
#include <set>
#include <cassert>
#include "graph.h"

nmap hook;
nmap coin;
nmap N;

#define MAX_SIZE 129205
#define TAIL (bool)0
#define HEAD (bool)1
#define all(x) x.begin(), x.end()
#define INVALID -1
#define CC2_DEBUG 0
#define MAIN_DEBUG 1
#define cilk_for1 for
#define DEBUG_ERR 0
typedef std::vector <int> :: iterator VI;
typedef std::vector <std::pair <int, int> >::iterator EI;


#define PRINT 0

#ifdef _DEBUG_
	bool visited[MAX_SIZE];
	std::vector <int> adj[MAX_SIZE];
#endif

#if 1
int* parallel_prefix(int *input, int size){
	int * output = (int *) malloc(sizeof(int)*size);
	int sum = 0;
	for(int i = 0; i < size; ++i){
		assert(input[i] >= 0);
		sum += input[i];
		output[i] = sum;
	}
	return output;
}
#endif

#if 0
int* parallel_prefix(int *input, int size){
	std::cout<<"Calling "<<size<<std::endl;
	if(size == 1) return input;
	int limit = size/2;
	if(size%2) limit++;
	assert(size > 0);
	int reduced_input[limit];
	int *reduced_output;
	int *output = (int *)malloc(sizeof(int)*size);
	std::cout<<size<<" PPREFIX"<<std::endl;
	if(!output){
		std::cout<<"Mem alloc failed"<<std::endl;
	}
	cilk_for1(int i = 0; i < limit; ++i){
		reduced_input[i] = input[2*i];
		if(2*i+1 < size)
			reduced_input[i]+= input[2*i+1];
	}
	reduced_output = parallel_prefix(reduced_input, limit);	
	std::cout<<"Returned "<<limit<<std::endl;
	cilk_for1(int i = 0; i < limit; ++i){
		int target = 2*i+1;
		if(target >= size){
			output[target - 1] = reduced_output[i];
		}
		else{
			output[target] = reduced_output[i];
			output[target - 1] = output[target] - input[target];
		}
	}
	return output;
}
#endif

#if 0
std::vector <int> select_root_vertex(std::vector <int> vertex, std::vector <int> L){
	std::vector <int>::iterator v_i;
	std::vector <int> root_vertex;
	for(v_i = vertex.begin(); v_i != vertex.end(); ++v_i){
		int u = *v_i;
		if(L[u] == u){
			#if 0
				std::cout<<"Retaining Root "<<u+1<<std::endl;
			#endif
			root_vertex.push_back(u);	
		}
		#ifdef _DEBUG_
		else{
			std::cout<<u+1<<" Docked here "<<L[u]+1<<std::endl;
		}
		#endif
	}
	return root_vertex;
}
#endif

vertex_set select_subset(const int * set, int * input, int n){
	/*Given a set, selection bit array, size and an unalloced refernce to subset.
	Allocates the subset and fills it with the correct values*/
	int * output = parallel_prefix(input, n);
	int limit = output[n-1];
	int * subset = (int *)malloc(sizeof(int) * limit);
	vertex_set v;
	cilk_for(int i = 0; i < n; ++i){
		if(input[i] == 1)
			subset[output[i]-1] = set[i];
	}
	#if DEBUG_ERR
	for(int i = 0; i < limit; ++i){
		std::cout<<i<<" "<<subset[i]<<std::endl;
	}
	#endif
	v.set = subset;
	v.limit = limit;
	free(output);
	return v;
}

vertex_set  select_root_vertex(vertex_set * vertex, nmap * L){
	std::vector <int>::iterator v_i;
	int *vset = vertex->set;
	int limit = vertex->limit;
	int * lset = L->set;
	vertex_set root_vertex;
	int *root_array;
	int * input = (int *) malloc(sizeof(int) * limit);
	//int input[limit];
	if(!input){
		std::cout<<"MEM alloc failed"<<std::endl;
	}

	cilk_for(int i = 0; i < limit; ++i){
		int u = vset[i];
		if(lset[u] == u){
			input[i] = 1;
		}
		else{
			input[i] = 0;
		}
	}
	#if DEBUG_ERR
	for(int i = 0; i < limit; ++i){
		int u = vset[i];
		int v = lset[u];
		if(input[i]){
			if(v != u)
				std::cout<<"ASSERT FAIL "<<i<<" "<<input[i]<<" "<<u<<" "<<lset[u]<<std::endl;
		}
		if(input[i])
			assert(lset[u] == u);	
	}
	#endif

	root_vertex= select_subset(vset, input, limit);
	root_array = root_vertex.set;

	/*
	int * output = parallel_prefix(input, limit);

	int root_size = output[limit-1];
	//int root_size = select_subset(vset, input, limit, root_array);
	root_array = (int *)malloc(sizeof(int) * root_size);

	cilk_for(int i = 0; i < limit; ++i){
		if(input[i] == 1){
			int index = output[i]-1;
			int u = vset[i];
			assert(index >= 0); assert(index < root_size);
			assert(u == lset[u]);
			root_array[index] = u;
		}
	}
	*/

	#if DEBUG_ERR
	for(int i = 0; i < root_size; ++i){
		int u = root_array[i];
		std::cout<<"DEBUG_ERR "<<i<<" "<<limit<<" "<<u<<" "<<lset[u]<<std::endl;
		assert(u == lset[u]);
	}
	std::cout<<" Releasing input/output"<<std::endl;
	std::cout<<" ROOT VERTEX DONE"<<std::endl;
	#endif

	free(input);
	return root_vertex;
}

std::vector <std::pair <int,int> > select_root_edge_cc3(std::vector<std::pair <int, int> > edge, std::vector <int> L){
	std::vector <std::pair <int, int> >root_edge;
	std::set <std::pair <int, int> > root_edge_set;

	std::vector <std::pair <int, int> >::iterator e_i;
	std::set <std::pair <int, int> >::iterator e_is;


	for(e_i = edge.begin(); e_i != edge.end(); ++e_i){
		int u = (*e_i).first;
		int v = (*e_i).second;
		if(L[u] != L[v] && L[L[u]] == L[u] && L[L[v]] == L[v])
			root_edge_set.insert(std::make_pair(L[u],L[v]));	
	}
	for(e_is = root_edge_set.begin(); e_is != root_edge_set.end(); ++e_is){
		root_edge.push_back(*e_is);
	}
	return root_edge;
}

edge_set select_root_edge(edge_set *edge, nmap * L){

	std::set <std::pair <int, int> > root_edge_set;
	std::set <std::pair <int, int> >::iterator e_is;
	int *uset = edge->uset;
	int *vset = edge->vset;
	int *lset = L->set;
	edge_set root_edge;

	for(int e_i = 0; e_i < edge->limit; ++e_i){
		int u = uset[e_i];
		int v = vset[e_i];
		if(lset[u] != lset[v])
			root_edge_set.insert(std::make_pair(lset[u],lset[v]));	
	}

	int limit = root_edge_set.size();
	int * ru_set = (int *)malloc(sizeof(int) * limit);
	int * rv_set = (int *)malloc(sizeof(int) * limit);

	int i = 0;
	for(e_is = root_edge_set.begin(); e_is != root_edge_set.end(); ++e_is){
		ru_set[i] = (*e_is).first;
		rv_set[i] = (*e_is).second;
		i++;
	}
	set_edge(&root_edge, ru_set, rv_set, root_edge_set.size());
	return root_edge;
}

int binary_search(std::vector <int> v, size_t start, size_t limit, int val){
	//Find the left most occurence of value in a sorted vector v
	if(start >= limit) return -1;
	size_t mid = start+limit/2;
	if(v[mid] == val){
		if(mid == start || mid-1 >= start && v[mid-1] > val)
			return mid;
		return binary_search(v, start, mid, val);
	}
	if(v[mid] < val)
		return binary_search(v, start, mid, val);
	return binary_search(v, mid+1, limit, val);
}

#ifdef _DEBUG_
int dfs(int val){
	visited[val] = true;
	std::cout<<"Visting "<<val+1<<std::endl;
	int count = 0;
	for(int i = 0; i < adj[val].size(); ++i){
		int u = adj[val][i];
		if(!visited[u])
			count += dfs(u);
	}
	return count + 1;
}
#endif

#if CC2_DEBUG
	void print_edges(std::vector <std::pair <int,int> > edge){
		#if 0
		std::cout<<"EDGE SET ";
		std::cout<<edge.size();
			std::cout<<std::endl;
			std::vector <std::pair <int,int> > :: iterator ei;
			for(ei = edge.begin(); ei != edge.end(); ++ei){
				std::cout<<(*ei).first+1<<","<<(*ei).second+1<< " -- ";
			}
		std::cout<<std::endl;
		#endif
	}
#endif

#if CC2_DEBUG
	void print_vertices(std::vector <int> vertex){
		#if 0
		std::cout <<"VERTEX SET ";
		std::cout<<vertex.size();
		std::cout<<std::endl;
		std::vector <int> :: iterator vi;
		for(vi = vertex.begin(); vi != vertex.end(); ++vi){
			std::cout<< *vi+1 << "  ";
		}
		std::cout<<std::endl;
		#endif
	}
#endif


int random(int max){
	//returns a random number from 0 to max-1
	return rand()%max;	
}

void random_hook(vertex_set * vertex, nmap * L,
							nmap * N){
	//std::vector <int> :: iterator v_i;
	int *vset = vertex->set;
	int *lset = L->set;
	int *nset = N->set;
	int *cset = coin.set;
	int *hset = hook.set;

	cilk_for(int v_i =  0; v_i < vertex->limit; ++v_i){
		int u = vset[v_i];
		assert(u == lset[u]);
		cset[u] =  random(2);
		hset[u] = (int)false;
		#if 0
		std::cout<<"TOSS "<<u+1<<" "<<coin[u]<<std::endl;
		#endif
	}

	cilk_for(int v_i =  0; v_i < vertex->limit; ++v_i){
		int u = vset[v_i];
		int v = nset[u];
		if(v == INVALID) continue;
		if(cset[u] == TAIL && cset[v] == HEAD){
			#if 0
			std::cout<<"Hooking "<<u+1<<" "<<v+1<<std::endl;
			#endif
			hset[u] = hset[v] = true;
			lset[u] = v;
		}
	}

	cilk_for(int v_i =  0; v_i < vertex->limit; ++v_i){
		int u = vset[v_i];
		if(hset[u])
			cset[u] = HEAD;
		else{
			if(cset[u] == HEAD)
				cset[u] = TAIL;
			else
				cset[u] = HEAD;
		}
	}

	cilk_for(int v_i =  0; v_i < vertex->limit; ++v_i){
		int u = vset[v_i];
		int v = nset[u];
		if(v == INVALID) continue;
		if(cset[u] == TAIL && cset[v] == HEAD){
			#if 0
			std::cout<<"Hooking "<<u+1<<" "<<L[v]+1<<std::endl;
			#endif
			lset[u] = lset[v];
		}
	}
}

void randomized_cc2(vertex_set *vertex,
			edge_set *edge,
			nmap *L){

	int * n_set = N.set;
	//std::vector <std::pair <int, int> >::iterator e_i;
	//std::vector <int>::iterator v_i;
	vertex_set  root_vertex;
	edge_set  root_edge;

	#if MAIN_DEBUG
		std::cout<<"VERTEX "<<vertex->limit<<std::endl;
		std::cout<<"EDGE "<<edge->limit<<std::endl;
		//print_vertices(vertex);
		//print_edges(edge);
	#endif

	if(edge->limit == 0) return;

	//cilk_for(VI v_i = vertex.begin(); v_i != vertex.end(); ++v_i)
	int vsize = vertex->limit;
	cilk_for(int i = 0; i < vsize; ++i){
		int u = vertex->set[i];
		n_set[u] = INVALID;
	}

	cilk_for(int e_i = 0; e_i != edge->limit; ++e_i){
		int u = edge->uset[e_i];
		int v = edge->vset[e_i];

		n_set[u] = v;
		n_set[v] = u;
	}


	random_hook(vertex, L, &N);

	root_vertex = select_root_vertex(vertex, L);
	root_edge = select_root_edge(edge, L);

	randomized_cc2(&root_vertex, &root_edge, L);

	cilk_for(int i = 0; i < vsize; ++i){
		int u = vertex->set[i];
		#if CC2_DEBUG
			std::cout <<" CHANGING LABEL "<<u+1<<"  "<<L->set[u]+1<<" "<<L->set[L->set[u]]+1<<std::endl;
		#endif
		int root_index = L->set[u];
		L->set[u] = L->set[root_index];
	}

	free(root_vertex.set);
	free(root_edge.uset);
	free(root_edge.vset);
	//free vertex and edge set
}


