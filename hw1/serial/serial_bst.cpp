#include <iostream>
#include <queue>
#include <algorithm>
#include <vector>
#include <cstring>
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
	FOR(i,MAX_SIZE){
		cout<<i<<" "<<d[i]<<endl;
	}
	int n;
	int m;
	cin>>n;
	cin>>m;
	memset(arr,0,MAX_SIZE*MAX_SIZE);
	FOR(i,m){
		int src,dest;
		cin>>src>>dest;
		arr[src][dest] = arr[dest][src]= 1;
	}
	int source;
	cin>>source;
	queue <int> qin;
	d[source] = 0;
	qin.push(source);
	while(!qin.empty()){
		int u = qin.front();
		qin.pop();
		FOR(v,n){
			//for each vertex v adjancent to u
			if(arr[u][v] && UNEXPLORED(v)){
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
