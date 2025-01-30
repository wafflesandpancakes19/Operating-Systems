#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 100

// Circular buffer and tracking variables
unsigned int buffer[BUFFER_SIZE];
int in = 0, out = 0, count = 0;
int done = 0; // Flag to indicate producer has finished

// Mutex and condition variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

// Producer function
void *producer(void *arg) {
    FILE *input = fopen("input-part1.txt", "r");
    if (!input) {
        perror("Failed to open input file");
        pthread_exit(NULL);
    }

    unsigned int num;
    while (fscanf(input, "%u", &num) && num != 0) {
        pthread_mutex_lock(&mutex);

        while (count == BUFFER_SIZE)
            pthread_cond_wait(&not_full, &mutex);

        // Add number to buffer
        buffer[in] = num;
        in = (in + 1) % BUFFER_SIZE;
        count++;

        pthread_cond_signal(&not_empty);
        pthread_mutex_unlock(&mutex);
    }

    // Signal the end of production
    pthread_mutex_lock(&mutex);
    done = 1; // Set done flag to indicate producer is finished
    pthread_cond_signal(&not_empty); // Wake up consumer if waiting
    pthread_mutex_unlock(&mutex);

    fclose(input);
    pthread_exit(NULL);
}

// Consumer function
void *consumer(void *arg) {
    FILE *output = fopen("output-part1.txt", "w");
    if (!output) {
        perror("Failed to open output file");
        pthread_exit(NULL);
    }

    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait if buffer is empty and producer is not done
        while (count == 0 && !done)
            pthread_cond_wait(&not_empty, &mutex);

        // Break if buffer is empty and producer is done
        if (count == 0 && done) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Remove number from buffer
        unsigned int num = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;

        // Write to output file
        fprintf(output, "Consumed:[%u],Buffer-State:[", num);
        for (int i = 0; i < count; i++) {
            fprintf(output, "%u", buffer[(out + i) % BUFFER_SIZE]);
            if (i < count - 1) fprintf(output, ",");
        }
        fprintf(output, "]\n");

        pthread_cond_signal(&not_full);
        pthread_mutex_unlock(&mutex);

        if (num == 0) break; // Stop if 0 is consumed
    }

    fclose(output);
    pthread_exit(NULL);
}

// Main function
int main() {
    pthread_t producer_thread, consumer_thread;

    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);

    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&not_full);
    pthread_cond_destroy(&not_empty);

    return 0;
}
