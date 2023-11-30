#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h> 
#include <time.h>

#define MAX_FILES 100
#define MAX_READERS 10
#define MAX_WRITERS 10

// Structure to represent a file
typedef struct {
    char name[256];
    char content[1024];
    int size;
} File;

File files[MAX_FILES];
int file_count = 0;
int reader_count = 0;
int writer_count = 0;
int turn = 0;
sem_t reader_mutex, writer_mutex, file_mutex;

// Function to read a file
void *read_file(void *file_name) {
    char *filename = (char *)file_name;
    int file_index = -1;

    // Find the file in the array
    sem_wait(&file_mutex);
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, filename) == 0) {
            file_index = i;
            break;
        }
    }
    sem_post(&file_mutex);

    if (file_index == -1) {
        printf("File not found: %s\n", filename);
        return NULL;
    }

    // Acquire the reader lock
    sem_wait(&reader_mutex);
    reader_count++;
    if (reader_count == 1) {
        sem_wait(&writer_mutex);
    }
    sem_post(&reader_mutex);

    // Read the file
    printf("Reading %s of %d bytes with %d readers and %d writers present\n",
           filename, files[file_index].size, reader_count, writer_count);

    // Simulate reading using nanosleep (sleep for a while)
    struct timespec sleep_time;
    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;
    nanosleep(&sleep_time, NULL);

    // Release the reader lock
    sem_wait(&reader_mutex);
    reader_count--;
    if (reader_count == 0) {
        sem_post(&writer_mutex);
    }
    sem_post(&reader_mutex);

    return NULL;
}

// Function to write to a file
void *write_file(void *args) {
    char *filename = ((char **)args)[0];
    char *content = ((char **)args)[1];
    int file_index = -1;

    // Find the file in the array
    sem_wait(&file_mutex);
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].name, filename) == 0) {
            file_index = i;
            break;
        }
    }
    if (file_index == -1) {
        strcpy(files[file_count].name, filename);
        file_index = file_count;
        file_count++;
    }
    sem_post(&file_mutex);

    // Acquire the writer lock with bounded waiting
    while (1) {
        sem_wait(&writer_mutex);
        if (turn == 0) {
            turn = 1;
            break;
        }
        sem_post(&writer_mutex);
        usleep(10000);  // Add a small delay to avoid busy-waiting
    }
    writer_count++;

    // Write to the file
    printf("Writing to %s added %d bytes with %d readers and %d writers present\n",
           filename, (int)strlen(content), reader_count, writer_count);

    // Simulate writing using nanosleep (sleep for a while)
    struct timespec sleep_time;
    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;
    nanosleep(&sleep_time, NULL);

    // Append content to the file
    strcat(files[file_index].content, content);
    files[file_index].size += strlen(content);

    // Release the writer lock
    writer_count--;
    turn = 0;
    sem_post(&writer_mutex);

    return NULL;
}

int main() {
    pthread_t readers[MAX_READERS];
    pthread_t writers[MAX_WRITERS];
    char input[256];

    // Initialize semaphores
    sem_init(&reader_mutex, 0, 1);
    sem_init(&writer_mutex, 0, 1);
    sem_init(&file_mutex, 0, 1);

    while (1) {
        printf("Enter command (read/write/exit): ");
        fgets(input, sizeof(input), stdin);
        input[strlen(input) - 1] = '\0'; // Remove newline character

        if (strncmp(input, "read", 4) == 0) {
            char file_name[256];
            sscanf(input, "read %s", file_name);
            pthread_create(&readers[reader_count], NULL, read_file, file_name);
            pthread_join(readers[reader_count], NULL); // Wait for the reader thread to finish
        } else if (strncmp(input, "write", 5) == 0) {
            char file_name[256];
            char content[1024];
            int write_type;
            if (sscanf(input, "write %d %s %[^\n]", &write_type, file_name, content) == 3) {
                if (write_type == 1) {
                    char **args = malloc(2 * sizeof(char *));
                    args[0] = file_name;
                    args[1] = content;
                    pthread_create(&writers[writer_count], NULL, write_file, args);
                    pthread_join(writers[writer_count], NULL); // Wait for the writer thread to finish
                } else if (write_type == 2) {
                    pthread_create(&writers[writer_count], NULL, write_file, (char *[2]){file_name, content});
                    pthread_join(writers[writer_count], NULL); // Wait for the writer thread to finish
                } else {
                    printf("Invalid write command.\n");
                }
            } else {
                printf("Invalid write command.\n");
            }
        } else if (strcmp(input, "exit") == 0) {
            break;
        } else {
            printf("Invalid command.\n");
        }
    }

    // Join all threads
    for (int i = 0; i < reader_count; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < writer_count; i++) {
        pthread_join(writers[i], NULL);
    }

    // Clean up semaphores
    sem_destroy(&reader_mutex);
    sem_destroy(&writer_mutex);
    sem_destroy(&file_mutex);

    return 0;
}
