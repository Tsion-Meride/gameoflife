#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  // For usleep
#include <stdbool.h>

#define N 500 // Size of the grid
#define T 200 // Number of generations
#define NUM_THREADS 4 // Number of worker threads

int world[N][N];
int new_world[N][N];
bool update_ready = false; // Flag to indicate when the grid is updated

pthread_mutex_t lock;

void initialize_world() {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            world[i][j] = rand() % 2; // Randomly initialize cells
        }
    }
}

int count_live_neighbors(int x, int y) {
    int count = 0;
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) continue; // Skip the cell itself
            int ni = (x + i + N) % N; // Wrap around
            int nj = (y + j + N) % N; // Wrap around
            count += world[ni][nj];
        }
    }
    return count;
}

void *update_cells(void *arg) {
    int thread_id = *((int *)arg);
    int rows_per_thread = N / NUM_THREADS;
    int start_row = thread_id * rows_per_thread;
    int end_row = (thread_id == NUM_THREADS - 1) ? N : start_row + rows_per_thread;

    for (int i = start_row; i < end_row; i++) {
        for (int j = 0; j < N; j++) {
            int nNeigh = count_live_neighbors(i, j);
            new_world[i][j] = (world[i][j] && (nNeigh == 2 || nNeigh == 3)) || (!world[i][j] && nNeigh == 3);
        }
    }

    pthread_mutex_lock(&lock);
    update_ready = true; // Indicate that the update is ready
    pthread_mutex_unlock(&lock);

    return NULL;
}

void plot() {
    while (true) {
        pthread_mutex_lock(&lock);
        if (update_ready) {
            // Send the current state of the world to Gnuplot
            FILE *fp = fopen("output.dat", "w");
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    fprintf(fp, "%d ", world[i][j]);
                }
                fprintf(fp, "\n");
            }
            fclose(fp);
            system("gnuplot -e \"set term dumb; plot 'output.dat' matrix; pause 0.1\"");

            // Update the world with the new world
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    world[i][j] = new_world[i][j];
                }
            }
            update_ready = false; // Reset the flag
        }
        pthread_mutex_unlock(&lock);
        usleep(100000); // Sleep to reduce CPU usage
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    initialize_world(); // Master thread initializes the world
    pthread_mutex_init(&lock, NULL);

    // Create worker threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, update_cells, &thread_ids[i]);
    }

    // Start the plotting thread
    pthread_t plot_thread;
    pthread_create(&plot_thread, NULL, (void *)plot, NULL);

    for (int t = 0; t < T; t++) {
        // Wait for all threads to finish before the next iteration
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(threads[i], NULL);
        }
        usleep(1000); // Control the speed of updates (adjust as needed)
    }

    // Clean up
    pthread_cancel(plot_thread); // Optionally handle termination properly
    pthread_mutex_destroy(&lock);
    return 0;
}
