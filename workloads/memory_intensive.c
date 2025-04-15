#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MEM_SIZE_MB 256
#define MEM_SIZE_BYTES (MEM_SIZE_MB * 1024 * 1024)

// Function prototypes
void* thread_function(void* arg);

int main() {
    pthread_t threads[14];
    int i;

    for (i = 0; i < 14; ++i) {
        if (pthread_create(&threads[i], NULL, thread_function, NULL)) {
            fprintf(stderr, "Error creating thread %d\n", i);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (i = 0; i < 14; ++i) {
        pthread_join(threads[i], NULL);
    }
//	sleep(100);
    return 0;
}

// Thread function
void* thread_function(void* arg) {
    // Allocate memory
    int* memory_block = (int*)malloc(MEM_SIZE_BYTES);
	while(1){
    if (memory_block == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        pthread_exit(NULL);
    }

    // Perform load and store operations in a loop
    unsigned int i;
    for (i = 0; i < MEM_SIZE_BYTES / sizeof(int); ++i) {
        memory_block[i] = i;
        int temp = memory_block[i]; // Load operation
        (void)temp; // Avoid compiler warning about unused variable
    }
}
    // Free memory
    free(memory_block);

    pthread_exit(NULL);
}
