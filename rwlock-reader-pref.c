#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t mutex;       // Controls access to reader_count
sem_t rw_lock;     // Controls access to the shared resource for writers
int reader_count = 0; // Number of active readers

void *reader(void *arg) {
    int id = *(int *)arg;

    // Entry section for readers
    sem_wait(&mutex);    // Acquire mutex to modify reader_count
    reader_count++;
    if (reader_count == 1) {
        sem_wait(&rw_lock);  // First reader locks the resource
    }
    sem_post(&mutex);    // Release mutex after modifying reader_count

    // Reading section
    FILE *output = fopen("output-reader-pref.txt", "a");
    if (output) {
        fprintf(output, "Reading,Number-of-readers-present:%d\n", reader_count);
        fclose(output);
    }

    FILE *input = fopen("shared-file.txt", "r");
    if (input) {
        char line[256];
        while (fgets(line, sizeof(line), input)) {
            // Process each line from shared_file.txt if needed
        }
        fclose(input);
    }
    sleep(1);  // Simulate reading time

    // Exit section for readers
    sem_wait(&mutex);    // Acquire mutex to modify reader_count
    reader_count = reader_count - 1; // decrement the reader count
    if (reader_count == 0) {
        sem_post(&rw_lock);  // Last reader releases the lock
    }
    sem_post(&mutex);    // Release mutex after modifying reader_count
    
    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;

    // Entry section for writers
    sem_wait(&rw_lock);  // Only one writer (or no readers) can access

    // Writing section
    FILE *output = fopen("output-reader-pref.txt", "a");
    if (output) {
        fprintf(output, "Writing,Number-of-readers-present:%d\n", reader_count);
        fclose(output);
    }

    FILE *input = fopen("shared-file.txt", "a");
    if (input) {
        fprintf(input, "Hello world!\n");
        fclose(input);
    }
    sleep(2);  // Simulate writing time

    // Exit section for writers
    sem_post(&rw_lock);  // Release access for others

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    }

    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    pthread_t readers[n], writers[m];
    int ids[n > m ? n : m];

    for (int i = 0; i < n || i < m; i++) {
        ids[i] = i + 1;
    }

    sem_init(&mutex, 0, 1);     // Initialize mutex for reader count access
    sem_init(&rw_lock, 0, 1);   // Initialize rw_lock for writer access control

    // Create reader threads
    for (int i = 0; i < n; i++) {
        pthread_create(&readers[i], NULL, reader, &ids[i]);
    }

    // Create writer threads
    for (int i = 0; i < m; i++) {
        pthread_create(&writers[i], NULL, writer, &ids[i]);
    }

    // Join all threads
    for (int i = 0; i < n; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < m; i++) {
        pthread_join(writers[i], NULL);
    }

    return 0;
}
