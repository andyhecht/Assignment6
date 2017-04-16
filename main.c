#include <pthread.h> /*used in other parts of the assignment */
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>  /* for uint64  */
#include <time.h>    /* for clock_gettime */
//#include <atomic>    /*used in other parts of the assignment */
#define NUM_THREADS 8
#define numPoints 100000000

double step = 0.5/numPoints;

pthread_mutex_t mutex_lock;

double f(double x) {
  return (6.0/sqrt(1-x*x));
}

//parts a and b
//double pi = 0.0;

//part c
/*std::atomic<double> pi{0};

void add_to_pi(double bar) {
  auto current = pi.load();
  while (!pi.compare_exchange_weak(current, current + bar));
}*/

//part e?
double sum[NUM_THREADS];

double calculatePi(void* thread_id){
    long tid = (long)thread_id;
    double temp_pi = 0.0;

    double x = 0.0d + (tid * ((0.5)/NUM_THREADS));


    for (int i = 0; i < numPoints/NUM_THREADS; i++) {
        temp_pi = temp_pi + step*f(x);  // Add to local sum
        x = x + step;  // next x
    }

    pthread_mutex_lock(&mutex_lock);
        //pi += temp_pi;
        //add_to_pi(temp_pi);
        sum[tid] = temp_pi;
    pthread_mutex_unlock(&mutex_lock);
    printf("thread %ld's contribution: %f\n", tid, temp_pi);

    return temp_pi;
}

int main(int argc, char *argv[]) {
  //uint64_t execTime; /*time in nanoseconds */
  //struct timespec tick, tock;

//  clock_gettime(CLOCK_MONOTONIC_RAW, &tick);

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex_lock, NULL);
    double step = 0.5/numPoints;

   int rc;
   long t;
   for(t=0;t<NUM_THREADS;t++){
     printf("In main: creating thread %ld\n", t);
     rc = pthread_create(&threads[t], NULL, calculatePi, (void *)t);
     if (rc){
       printf("ERROR; return code from pthread_create() is %d\n", rc);
       exit(-1);
       }
     }

 // clock_gettime(CLOCK_MONOTONIC_RAW, &tock);

 //execTime = 1000000000 * (tock.tv_sec - tick.tv_sec) + tock.tv_nsec - tick.tv_nsec;

 //printf("elapsed process CPU time = %llu nanoseconds\n", (long long unsigned int) execTime);


    for(int i = 0; i < NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_lock(&mutex_lock);
        //for part with array
        double pi = 0.0;
        for(int i = 0; i < NUM_THREADS; i++){
            pi += sum[i];
        }
        printf("pi = %.20f\n", pi);
        //part c
        //auto current = pi.load();
        //printf("%.20f\n", (double)current);
    pthread_mutex_unlock(&mutex_lock);

  return 0;
}
