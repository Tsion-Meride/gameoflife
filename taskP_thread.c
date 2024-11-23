#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>  // For sleep
#include <stdbool.h>

#define N 500 // Size of the grid
#define T 200 // Number of generations

int world[N][N];
int new_world[N][N];
pthread_mutex_t lock;
bool update_ready = false; // Flag to indicate when the grid is updated

void initialize_world() {
    // Initialize the grid with random values
}

int count_live_neighbors(int x, int y) {
    // Count live neighbors of cell (x, y)
    // Implement neighbor counting logic
}

void *update_cells(void *arg) {
    for (int t = 0; t < T; t++) {
        pthread_mutex_lock(&lock);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                int nNeigh = count_live_neighbors(i, j);
                // Apply the Game of Life rules
                new_world[i][j] = (world[i][j] && (nNeigh == 2 || nNeigh == 3)) || (!world[i][j] && nNeigh == 3);
            }
        }
        update_ready = true; // Indicate that the update is ready
        pthread_mutex_unlock(&lock);
        sleep(1); // Control the speed of updates
    }
    return NULL;
}

void *plot(void *arg) {
    while (true) {
        pthread_mutex_lock(&lock);
        if (update_ready) {
            // Send the current state of the world to Gnuplot
            // Example: write to a file and call gnuplot
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
    return NULL;
}

int main() {
    initialize_world();
    pthread_t update_thread, plot_thread;
    pthread_mutex_init(&lock, NULL);

    // Create threads
    pthread_create(&update_thread, NULL, update_cells, NULL);
    pthread_create(&plot_thread, NULL, plot, NULL);

    // Wait for threads to finish
    pthread_join(update_thread, NULL);
    pthread_cancel(plot_thread); // Optionally handle termination properly

    pthread_mutex_destroy(&lock);
    return 0;
}