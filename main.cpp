#include <cstdio>
#include <cmath>

#include <ctime>
#include <pthread.h>

#include <atomic>

#include "bellman.h"

#define NUM_THREADS 1
#define NUM_POINTS 100000000
#define STEP (0.5 / NUM_POINTS)

pthread_mutex_t mutex_lock;

double f(double x) {
	return 6.0 / sqrt(1 - x * x);
}

//parts a and b
//double pi = 0.0;

//part c
std::atomic<double> pi {0};

void add_to_pi(double bar) {
	auto current = pi.load();
	while (!pi.compare_exchange_weak(current, current + bar));
}

//parts e/f
double sum[NUM_THREADS];

void *calculatePi(void *thread_id) {
	long tid = (long)thread_id;

//	double temp_pi = 0.0;

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
	pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
	unsigned long long int execTime; // time in nanoseconds

	struct timespec tick, tock;
	pthread_t threads[NUM_THREADS];
	pthread_mutex_init(&mutex_lock, NULL);

	int rc;
	long t;

	clock_gettime(CLOCK_MONOTONIC_RAW, &tick);

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

	pthread_mutex_destroy(&mutex_lock);

//	for part with array
//	double pi = 0.0d;
//	for (int i = 0; i < NUM_THREADS; i++) pi += sum[i];


	clock_gettime(CLOCK_MONOTONIC_RAW, &tock);

	execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;

	printf("elapsed process CPU time = %llu ns\n", execTime);

	// printf("pi = %.20f\n", pi);
	// part c
	auto current = pi.load();
	printf("%.20f\n", (double)current);


//////////////////////////////////SSSP//////////////////////////////////
	FILE *fp = fopen("results.out", "w");

	sssp((char *)"rmat15.dimacs", 0, fp);
	sssp((char *)"rmat23.dimacs", 0, fp);
	sssp((char *)"road-NY.dimacs", 140960, fp);
	sssp((char *)"road-FLA.dimacs", 316606, fp);

	fclose(fp);

	return 0;
}


