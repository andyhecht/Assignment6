#include <climits>
#include <cstring>

#include <ctime>
#include <pthread.h>

#include "bellman.h"

#define THREAD_NUM 8

pthread_barrier_t barrier;
pthread_barrier_t barrier2;

pthread_mutex_t relax_lock;
pthread_mutex_t sync_lock;

bool conv = false;

bool converge_global[THREAD_NUM];

Graph *read_dimacs(char* file) {
	int V, E, u, v, w;

	FILE *fp = fopen(file, "r");
	fscanf(fp, "p sp %d %d ", &V, &E);

	int *length = new int[V] (), count = 0;

	for (int i = 0; i < E; i++) {
		fscanf(fp, "a %d %d %d ", &u, &v, &w);
		if (u == v) continue;
		length[u - 1]++;
		count++;
	}

	rewind(fp);
	fscanf(fp, "p sp %d %d ", &V, &E);
	E = count; // more accurate value due to dimacs issues

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

	pthread_barrier_init(&barrier, NULL, THREAD_NUM);
	pthread_barrier_init(&barrier2, NULL, THREAD_NUM);
	pthread_mutex_init(&relax_lock, NULL);
	pthread_mutex_init(&sync_lock, NULL);

	return G;
}

void assign_nodes(Worker_Data *t_data, Graph *G) {
	int V = G->V, E = G->E, *length = G->length;

	int step = (E / THREAD_NUM), count = 0, total = 0;
	int div[THREAD_NUM] = {0};

	for (int i = 0, j = 0; i < V - 1 && j < THREAD_NUM - 1; i++) {
		count += length[i];
		total += length[i];
		if (count > 0 && ((count / step) >= 1 || (step - count) <= step*.05)) {
			div[++j] = i;

			// adjust step size for oversteps
			if (count > step)
				step = (E - (count - step + total)) / (THREAD_NUM - j);
			count = 0;
		}
	}

	for (int i = 0; i < THREAD_NUM; i++) {
		int lo = (i) ? div[i] + 1 : 0;
		int hi = (i == THREAD_NUM - 1) ? V - 1 : div[i + 1];

		t_data[i].G = G, t_data[i].tid = i;
		t_data[i].lo = lo, t_data[i].hi = hi;
	}
}

int *bf_serial(Graph *G, int src) {
	int V = G->V, *length = G->length, *dist = new int[V];
	Edge **edge = G->edge;
	bool converge = true;

	for (int i = 0; i < V; dist[i++] = INT_MAX);
	dist[src] = 0;

	for (int i = 1; i < V; i++) {
		for (int j = 0; j < V; j++) {
			for (int k = 0; k < length[j]; k++) {
				int u = j, v = edge[j][k].v, w = edge[j][k].w;
				if (dist[u] == INT_MAX) break;

				if (dist[v] > dist[u] + w) {
					dist[v] = dist[u] + w;
					converge = false;
				}
			}
		}
		if (converge) break;
		converge = true;
	}

	return dist;
}

int *bf_parallel(Graph *G, int src, Worker_Data *t_data) {
	int V = G->V, rc, *dist = new int[V];
	for (int i = 0; i < V; dist[i++] = INT_MAX);
	dist[src] = 0;

	pthread_t tid[THREAD_NUM];


	for (int i = 0; i < THREAD_NUM; i++) {
		t_data[i].dist = dist, t_data[i].src = src;
		rc = pthread_create(&tid[i], NULL, bf_routine, (void *)&t_data[i]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			return NULL;
		}
	}

	for (int i = 0; i < THREAD_NUM; i++)
		pthread_join(tid[i], NULL);

	return dist;
}

void *bf_routine(void *thread_data) {
	Worker_Data *data = (Worker_Data *)thread_data;
	int lo = data->lo, hi = data->hi, *dist = data->dist, tid = data->tid;

	Graph *G = data->G;
	int *length = G->length, ret;
	Edge **edge = G->edge;

	bool converge = false;

	do {
		converge = false;
		for (int j = lo; j < hi + 1; j++) {
			for (int k = 0; k < length[j]; k++) {
				int u = j, v = edge[j][k].v, w = edge[j][k].w;
				if (dist[u] == INT_MAX) break;

				if (dist[v] > dist[u] + w) {
				pthread_mutex_lock(&relax_lock);
					dist[v] = dist[u] + w;
				pthread_mutex_unlock(&relax_lock);
					converge = true;
				}
			}
		}
		converge_global[tid] = converge;
		ret = pthread_barrier_wait(&barrier);

		if (ret == PTHREAD_BARRIER_SERIAL_THREAD) {
			pthread_mutex_lock(&sync_lock);
			converge = false;
			for (int i = 0; i < THREAD_NUM; i++) {
				if (converge_global[i]) {
					converge = true;
					break;
				}
			}
			conv = converge;
			pthread_mutex_unlock(&sync_lock);
		}

		pthread_barrier_wait(&barrier2);

	} while (conv);

	pthread_exit(NULL);
}

void sssp(char *file, int src, FILE *fp) {
	Graph *G = read_dimacs(file);

	Worker_Data t_data[THREAD_NUM];
	assign_nodes(t_data, G);

	unsigned long long int exec_time;
	struct timespec tick, tock;

	// SERIAL BELLMAN-FORD
	clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
	int *norm = bf_serial(G, src);
	clock_gettime(CLOCK_MONOTONIC_RAW, &tock);

	exec_time = 1000000000 * (tock.tv_sec - tick.tv_sec) +
		tock.tv_nsec - tick.tv_nsec;

	fprintf(fp, "Serial Bellman-Ford Results for %s\nElapsed Time: %llu ns\n", file, exec_time);

	delete []norm;


	// PARALLEL BELLMAN-FORD
	clock_gettime(CLOCK_MONOTONIC_RAW, &tick);
	int *para = bf_parallel(G, src, t_data);
	clock_gettime(CLOCK_MONOTONIC_RAW, &tock);

	exec_time = 1000000000 * (tock.tv_sec - tick.tv_sec) +
			tock.tv_nsec - tick.tv_nsec;

	fprintf(fp, "\nParallel Bellman-Ford Results for %s (%d-Threads)\nElapsed Time: %llu ns\n",
			file, THREAD_NUM, exec_time);

	for (int i = 0; i < G->V; i++)
		fprintf(fp, "%d %d\n", i+1, para[i]);
	fprintf(fp, "\n\n");

	pthread_barrier_destroy(&barrier);
	pthread_barrier_destroy(&barrier2);
	pthread_mutex_destroy(&relax_lock);
	pthread_mutex_destroy(&sync_lock);

	delete []para;

	delete G;
}

