#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <unistd.h>  // For usleep
#include <stdbool.h>

#define N 500 // Size of the grid
#define T 200 // Number of generations

int world[N][N];
int new_world[N][N];
bool update_ready = false; // Flag to indicate when the grid is updated

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

void update_cells(int num_threads) {
    #pragma omp parallel for schedule(static) // Use static scheduling for even distribution
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            // Each thread updates its assigned rows
            int nNeigh = count_live_neighbors(i, j);
            new_world[i][j] = (world[i][j] && (nNeigh == 2 || nNeigh == 3)) || (!world[i][j] && nNeigh == 3);
        }
    }
    update_ready = true; // Indicate that the update is ready
}

void plot() {
    while (true) {
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
            #pragma omp critical
            {
                for (int i = 0; i < N; i++) {
                    for (int j = 0; j < N; j++) {
                        world[i][j] = new_world[i][j];
                    }
                }
                update_ready = false; // Reset the flag
            }
        }
        usleep(100000); // Sleep to reduce CPU usage
    }
}

int main() {
    initialize_world(); // Master thread initializes the world
    
    // Start the plotting thread
    #pragma omp parallel
    {
        #pragma omp single
        {
            plot();
        }
    }

    for (int t = 0; t < T; t++) {
        update_cells(omp_get_max_threads()); // Update cells concurrently
        usleep(1000); // Control the speed of updates (adjust as needed)
    }

    return 0;
}