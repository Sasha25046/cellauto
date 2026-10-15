#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <string>
#include "common.h"

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int N = 1000;
    double d = 0.60;
    uint64_t seed = 42ULL;
    if (argc >= 2) N = std::stoi(argv[1]);
    if (argc >= 3) d = std::stod(argv[2]);
    if (argc >= 4) seed = std::stoull(argv[3]);

    if (N % size != 0) {
        if (rank == 0) std::cerr << "Помилка: N (" << N << ") має ділитися на P (" << size << ")\n";
        MPI_Finalize();
        return 1;
    }

    const int H = N / size;
    const int r_start = rank * H;
    const int up   = (rank == 0)        ? MPI_PROC_NULL : rank - 1;
    const int down = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    std::vector<uint8_t> cur((size_t)(H + 2) * N, 0), nxt((size_t)(H + 2) * N, 0);

    for (int li = 1; li <= H; ++li) {
        int gi = r_start + (li - 1);
        for (int j = 0; j < N; ++j)
            if (is_tree(seed, gi, j, d))
                cur[(size_t)li * N + j] = (j == 0) ? 2 : 1;
    }

    
    MPI_Barrier(MPI_COMM_WORLD);
    double t_start = MPI_Wtime();

    int step = 0;
    int local_right = 0;
    long local_burnt = 0;

    while (true) {
        MPI_Sendrecv(&cur[(size_t)1 * N], N, MPI_UINT8_T, up, 2,
                     &cur[(size_t)(H + 1) * N], N, MPI_UINT8_T, down, 2,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Sendrecv(&cur[(size_t)H * N], N, MPI_UINT8_T, down, 1,
                     &cur[0], N, MPI_UINT8_T, up, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        long local_burning = 0;
        for (int li = 1; li <= H; ++li) {
            for (int j = 0; j < N; ++j) {
                uint8_t s = cur[(size_t)li * N + j];
                uint8_t ns = s;
                if (s == 2) {
                    ns = 3;
                    local_burnt++;
                    if (j == N - 1) local_right = 1;
                } else if (s == 1) {
                    bool nb = (cur[(size_t)(li - 1) * N + j] == 2) ||
                              (cur[(size_t)(li + 1) * N + j] == 2) ||
                              (j > 0     && cur[(size_t)li * N + (j - 1)] == 2) ||
                              (j < N - 1 && cur[(size_t)li * N + (j + 1)] == 2);
                    if (nb) { ns = 2; local_burning++; }
                }
                nxt[(size_t)li * N + j] = ns;
            }
        }
        cur.swap(nxt);
        step++;

        long global_burning = 0;
        MPI_Allreduce(&local_burning, &global_burning, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
        if (global_burning == 0) break;
    }

    double local_elapsed = MPI_Wtime() - t_start;
    double max_elapsed = 0.0;
    MPI_Reduce(&local_elapsed, &max_elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    long g_burnt = 0;
    MPI_Reduce(&local_burnt, &g_burnt, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    int g_right = 0;
    MPI_Reduce(&local_right, &g_right, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    unsigned long long local_cs = 0, g_cs = 0;
    for (int li = 1; li <= H; ++li)
        for (int j = 0; j < N; ++j)
            local_cs += cell_checksum(r_start + li - 1, j, cur[(size_t)li * N + j]);
    MPI_Reduce(&local_cs, &g_cs, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        std::cout << "=== V1 (Parallel MPI) Finished ===\n";
        std::cout << "Processes P: " << size << "\n";
        std::cout << "Grid size N: " << N << " x " << N << "\n";
        std::cout << "Steps: " << step << "\n";
        std::cout << "Total burnt cells: " << g_burnt << "\n";
        std::cout << "Reached right edge: " << (g_right ? "YES" : "NO") << "\n";
        std::cout << "Checksum: " << g_cs << "\n";
        std::cout << "Execution time: " << std::fixed << std::setprecision(4) << max_elapsed << " seconds\n";
    }
    MPI_Finalize();
    return 0;
}