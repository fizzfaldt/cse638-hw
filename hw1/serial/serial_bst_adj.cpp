#include <iostream>
#include <queue>
#include <algorithm>
#include <vector>
#include <cstring>
#include <iterator>
#define MAX_SIZE 15
#define FOR(i,n) for(int i=0;i<n;i++)
#define UNEXPLORED(u) d[u]<0

using namespace std;

int main(){
	bool arr[MAX_SIZE][MAX_SIZE];
	int d[MAX_SIZE];
	FOR(i,MAX_SIZE){
		d[i] = -1;
	}
	int n;
	int m;
	cin>>n;
	cin>>m;
	vector<int> adj[n];
	memset(arr,0,MAX_SIZE*MAX_SIZE);
	FOR(i,m){
		int src,dest;
		cin>>src>>dest;
		adj[src].push_back(dest);
		adj[dest].push_back(src);
	}
	int source;
	cin>>source;
	queue <int> qin;
	d[source] = 0;
	qin.push(source);
	while(!qin.empty()){
		int u = qin.front();
		qin.pop();
		FOR(i,adj[u].size()){
			//for each vertex v adjancent to u
			int v = adj[u][i];
			if(UNEXPLORED(v)){
				d[v] = d[u] + 1;
				qin.push(v);
			}
		}
	}
	FOR(i,n){
		cout<<i<<" "<<d[i]<<endl;
	}
	return 0;
}
