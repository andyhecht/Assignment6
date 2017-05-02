#ifndef BELLMAN_H
#define BELLMAN_H

using namespace std;

struct Edge {
	int v, w;
	Edge(int v, int w) : v(v), w(w) {}
	Edge() {}
};

struct Graph {
	int V, E, *length;
	Edge **edge;

	Graph(int V, int E, int *length) : V(V), E(E), length(length), edge(new Edge*[V]) { }
	~Graph() {
		for (int i = 0; i < V; delete[] edge[i++]);
	}
};

struct Worker_Data {
	int src, lo, hi;
	int *dist;
	Graph *G;
};

Graph *read_dimacs(char *);

int *bellman_ford(Graph *, int);

int *bf_parallel(Graph *, int, int);

void *bf_routine(void *);

#endif
