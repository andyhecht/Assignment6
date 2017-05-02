#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>

#include "bellman.h"

pthread_barrier_t barrier;
pthread_mutex_t relax_lock;

Graph *read_dimacs(char* file) {
	int V, E, u, v, w;

	FILE *fp = fopen(file, "r");
	fscanf(fp, "p sp %d %d ", &V, &E);

	int *length = new int[V] ();

	for (int i = 0; i < E; i++) {
		fscanf(fp, "a %d %d %d ", &u, &v, &w);
		if (u == v) continue;
		length[u - 1]++;
	}

	rewind(fp);
	fscanf(fp, "p sp %d %d ", &V, &E);

	Graph *G = new Graph(V, E, length);
	G->edge = new Edge*[V];
	for (int i = 0; i < V; i++)
		G->edge[i] = new Edge[length[i]]();

	memset(length, 0, sizeof(*length) * V);

	for (int i = 0; i < E; i++) {
		fscanf(fp, "a %d %d %d ", &u, &v, &w);
		if (u == v) continue;
		G->edge[u - 1][length[u-1]].v = v - 1;
		G->edge[u - 1][length[u-1]++].w = w;
	}

	fclose(fp);

	return G;
}

int *bellman_ford(Graph *G, int src) {
	int V = G->V, E = G->E, *length = G->length, *dist = new int[V];
	Edge **edge = G->edge;
	bool conv = false;

	for (int i = 0; i < V; dist[i++] = INT_MAX);
	dist[src] = 0;

	for (int i = 1; i < V; i++) {
		for (int j = 0; j < V; j++) {
			for (int k = 0; k < length[j]; k++) {
				int u = j, v = edge[j][k].v, w = edge[j][k].w;
				if (dist[u] != INT_MAX && dist[v] > dist[u] + w)
					dist[v] = dist[u] + w;
			}
		}
	}

	return dist;
}

int *bf_parallel(Graph *G, int src, int num_threads) {
	int V = G->V, E = G->E, step = V / num_threads;
	int hi, lo, rc;

	int *dist = new int[V];
	for (int i = 0; i < V; dist[i++] = INT_MAX);
	dist[src] = 0;

	pthread_t tid[num_threads];
	Worker_Data t_data[num_threads];

	pthread_barrier_init(&barrier, NULL, num_threads);
	pthread_mutex_init(&relax_lock, NULL);

	for (int i = 0; i < num_threads; i++) {
		lo = (i) ? hi + 1 : 0;
		hi = (i == num_threads - 1) ? V : lo + step - 1;

		t_data[i].src = src, t_data[i].G = G, t_data[i].dist = dist;
		t_data[i].lo = lo, t_data[i].hi = hi;

		rc = pthread_create(&tid[i], NULL, bf_routine, (void *)&t_data[i]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			return NULL;
		}
	}

	int status;
	for (int i = 0; i < num_threads; i++) {
		rc = pthread_join(tid[i], NULL);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			return NULL;
		}
	}

	pthread_barrier_destroy(&barrier);
	pthread_mutex_destroy(&relax_lock);

	return dist;
}

void *bf_routine(void *thread_data) {
	Worker_Data *data = (Worker_Data *)thread_data;

	int *dist = data->dist;
	int src = data->src, lo = data->lo, hi = data->hi;
	Graph *G = data->G;

	printf("lo: %d, hi: %d\n", lo, hi);

	int V = G->V, E = G->E, *length = G->length;
	Edge **edge = G->edge;

	for (int i = 1; i < V; i++) {
		for (int j = lo; j < hi + 1; j++) {
			for (int k = 0; k < length[j]; j++) {
				int u = j, v = G->edge[j][k].v, w = G->edge[j][k].w;
				/*
				int old_u = dist[u].load(std::memory_order_relaxed),
					old_v = dist[v].load(std::memory_order_relaxed);

				if (old_v != INT_MAX && old_v > old_u + w)
					while (old_v > old_u + w && !dist[v].compare_exchange_weak(old_v, old_u + w));
				*/
				pthread_mutex_lock(&relax_lock);
				if (dist[v] != INT_MAX && dist[v] > dist[u] + w)
					dist[v] = dist[u] + w;
				pthread_mutex_unlock(&relax_lock);
			}
		}
		pthread_barrier_wait(&barrier);
	}
}

