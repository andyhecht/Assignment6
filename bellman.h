#ifndef BELLMAN_H
#define BELLMAN_H

#include <cstdio>

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
	int src, lo, hi, tid;
	int *dist;
	Graph *G;
};

Graph *read_dimacs(char *);

void assign_nodes(Worker_Data *);

int *bf_serial(Graph *, int);

int *bf_parallel(Graph *, int, Worker_Data *);

void *bf_routine(void *);

void sssp(char *, int, FILE *);

#endif
