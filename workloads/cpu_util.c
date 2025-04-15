#include <stdio.h>
#include <pthread.h>
#include <math.h>
#define NUM_THREADS 2
#define ITERATIONS   1000000000 // Adjust the number of iterations to control CPU load

void *compute(void *arg) {
	double result;
    	long tid;
	while(1){
    // Perform some computation
	   for (int i = 0; i < ITERATIONS; i++) {
		    result += pow(sin(i), i) + tid; // Some heavy arbitrary computation
	   }
	}
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    long t;

    // Create threads
    for (t = 0; t < NUM_THREADS; t++) {
        printf("Creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, compute, (void *)t);
        if (rc) {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            return -1;
        }
    }

    // Join threads
    for (t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    return 0;
}
