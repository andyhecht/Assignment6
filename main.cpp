#include <stdio.h>
#include <math.h>

#include <time.h>
#include <pthread.h>

#include <atomic>

#include "bellman.h"

#define NUM_THREADS 1
#define NUM_POINTS 100000000
#define STEP (0.5 / NUM_POINTS)

using namespace std;

pthread_mutex_t mutex_lock;

double f(double x) {
	return 6.0 / sqrt(1 - x * x);
}

//parts a and b
//double pi = 0.0;

//part c
atomic<double> pi {0};

void add_to_pi(double bar) {
	auto current = pi.load();
	while (!pi.compare_exchange_weak(current, current + bar));
}

//parts e/f
double sum[NUM_THREADS];

void *calculatePi(void *thread_id) {
	long tid = (long)thread_id;

	double temp_pi = 0.0;

	double x = 0.0 + (tid * ((0.5)/NUM_THREADS));

	for (int i = 0; i < NUM_POINTS / NUM_THREADS; i++) {
		// temp_pi = temp_pi + STEP*f(x);  // Add to local sum
		// pthread_mutex_lock(&mutex_lock);
		// sum[tid] = sum[tid] +  STEP*f(x);
		// pi = pi + STEP*f(x);
		add_to_pi(STEP * f(x));
		// pthread_mutex_unlock(&mutex_lock);
		x = x + STEP;  // next x
	}

	// sum[tid] = temp_pi;
	// printf("thread %ld's contribution: %f\n", tid, temp_pi);

}

int main(int argc, char *argv[]) {
	unsigned long long int execTime; // time in nanoseconds

	struct timespec tick, tock;
	pthread_t threads[NUM_THREADS];
	pthread_mutex_init(&mutex_lock, NULL);

	int rc;
	long t;


	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tick);

	for (t = 0; t < NUM_THREADS; t++) {
		// printf("In main: creating thread %ld\n", t);
		rc = pthread_create(&threads[t], NULL, calculatePi, (void *)&t);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			return -1;
		}
	}

	for(int i = 0; i < NUM_THREADS; i++)
		pthread_join(threads[i], NULL);

//	for part with array
//	double pi = 0.0d;
//	for (int i = 0; i < NUM_THREADS; i++) pi += sum[i];


	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tock);

	execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;

	printf("elapsed process CPU time = %llu ns\n", execTime);

	// printf("pi = %.20f\n", pi);
	// part c
	auto current = pi.load();
	printf("%.20f\n", (double)current);

//////////////////////////////////SSSP//////////////////////////////////
	int thread_n = 8;
	char *file = (char *)"rmat15.dimacs";
	FILE *fp = fopen("results.out", "w");
	Graph *G = read_dimacs(file);


	// NORMAL BELLMAN-FORD
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tick);
	int *norm = bellman_ford(G, 0);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tock);

	unsigned long long int normal_time = 1000000000 * (tock.tv_sec - tick.tv_sec) +
		tock.tv_nsec - tick.tv_nsec;

	fprintf(fp, "Normal Bellman-Ford Results\nElapsed Time: %llu ns\n", normal_time);
	for (int i = 0; i < G->V; i++)
//		if (G->length[i])
			fprintf(fp, "%d %d\n", i+1, norm[i]);


	// PARALLEL BELLMAN-FORD
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tick);
	int *para = bf_parallel(G, 0, thread_n);
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tock);

	unsigned long long int parallel_time = 1000000000 * (tock.tv_sec - tick.tv_sec) +
		tock.tv_nsec - tick.tv_nsec;

	fprintf(fp, "\nParallel Bellman-Ford Results (%d-Threads)\nElapsed Time: %llu ns\n",
			thread_n, parallel_time);
	for (int i = 0; i < G->V; i++)
//		if (G->length[i])
			fprintf(fp, "%d %d\n", i+1, para[i]);


	// ACCURACY TEST
	int j = 0;
	for (int i = 0; i < G->V; i++) {
		if (para[i] != norm[i] && ++j) {
			printf("BROKEN AT i = %d\n", i+1);
			break;
		}
	}
	if (!j) printf("WORKS");

	delete []para;
	delete []norm;

	delete G;

	fclose(fp);

	return 0;
}
