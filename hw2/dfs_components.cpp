#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#define MAX_SIZE 129205
#define DEBUG 0

using namespace std;

vector <bool> visited;
vector <int> component;
vector <vector <int> >adj;

int dfs(int val, int comp){
	component[val] = comp;
	visited[val] = true;
	int count = 0;
	for(int i = 0; i < adj[val].size(); ++i){
		int u = adj[val][i];
		if(!visited[u])
			count += dfs(u, comp);
	}
	return count + 1;
}

int main(){
	int n,m;
	cin>>n>>m;
	component.resize(n);
	visited.resize(n);
	adj.resize(n);
	int comp = 0;
	vector <int> comp_arr;
	for(int i = 0; i < m; ++i){
		int a, b;
		cin>>a>>b;
		a--;b--;
		adj[a].push_back(b);
		adj[b].push_back(a);
	}
	for(int i = 0; i < n; ++i){
		if(!visited[i]){
			int val = dfs(i, i);
			comp_arr.push_back(val);
			++comp;
		}
	}
	cout<<comp<<endl;
	sort(comp_arr.begin(), comp_arr.end());
	for(int i = comp_arr.size()-1; i >= 0; --i){
		cout<<comp_arr[i]<<endl;
	}
	#if DEBUG
	for(int i = 0; i < n; ++i){
		cout<<i<<" "<<component[i]<<endl;
	}
	#endif
}
