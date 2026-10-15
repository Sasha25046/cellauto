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

    int N = 1200;
    double d = 0.60;
    uint64_t seed = 42ULL;
    int profile = 0;
    if (argc >= 2) N = std::stoi(argv[1]);
    if (argc >= 3) d = std::stod(argv[2]);
    if (argc >= 4) seed = std::stoull(argv[3]);
    if (argc >= 5) profile = std::stoi(argv[4]);

    if (N % size != 0) {
        if (rank == 0) std::cerr << "Помилка: N має ділитися на P\n";
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
    const double t_total_start = MPI_Wtime();

    double t_comm = 0.0, t_calc = 0.0, t_reduce = 0.0, t_sync = 0.0;
    int step = 0;
    int local_right = 0;
    long local_burnt = 0;

    while (true) {
        if (profile) {
            double s0 = MPI_Wtime();
            MPI_Barrier(MPI_COMM_WORLD);
            t_sync += MPI_Wtime() - s0;
        }

        double c0 = MPI_Wtime();
        MPI_Request reqs[4];
        int rc = 0;
        MPI_Irecv(&cur[0],                     N, MPI_UINT8_T, up,   1, MPI_COMM_WORLD, &reqs[rc++]);
        MPI_Irecv(&cur[(size_t)(H + 1) * N],   N, MPI_UINT8_T, down, 2, MPI_COMM_WORLD, &reqs[rc++]);
        MPI_Isend(&cur[(size_t)1 * N],         N, MPI_UINT8_T, up,   2, MPI_COMM_WORLD, &reqs[rc++]);
        MPI_Isend(&cur[(size_t)H * N],         N, MPI_UINT8_T, down, 1, MPI_COMM_WORLD, &reqs[rc++]);
        MPI_Waitall(rc, reqs, MPI_STATUSES_IGNORE);
        t_comm += MPI_Wtime() - c0;

        double k0 = MPI_Wtime();
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
        t_calc += MPI_Wtime() - k0;

        if (profile) {
            double s0 = MPI_Wtime();
            MPI_Barrier(MPI_COMM_WORLD);
            t_sync += MPI_Wtime() - s0;
        }
        double r0 = MPI_Wtime();
        long global_burning = 0;
        MPI_Allreduce(&local_burning, &global_burning, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
        t_reduce += MPI_Wtime() - r0;

        if (global_burning == 0) break;
    }

    const double t_total = MPI_Wtime() - t_total_start;

    double loc[5] = {t_total, t_calc, t_comm, t_sync, t_reduce};
    double mx[5]  = {0, 0, 0, 0, 0};
    MPI_Reduce(loc, mx, 5, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

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
        auto pct = [&](double x) { return x / mx[0] * 100.0; };
        std::cout << "\n=== V3 (P=" << size << ", N=" << N << ", profile=" << profile << ") ===\n";
        std::cout << "Steps: " << step << ", burnt: " << g_burnt
                  << ", right edge: " << (g_right ? "YES" : "NO") << ", checksum: " << g_cs << "\n";
        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Total:   " << mx[0] << " s (100%)\n";
        std::cout << "Compute: " << mx[1] << " s (" << pct(mx[1]) << "%)\n";
        std::cout << "Halo:    " << mx[2] << " s (" << pct(mx[2]) << "%)\n";
        std::cout << "Sync:    " << mx[3] << " s (" << pct(mx[3]) << "%)\n";
        std::cout << "Reduce:  " << mx[4] << " s (" << pct(mx[4]) << "%)\n";
        std::cout << "CSV," << size << "," << N << "," << step << "," << std::setprecision(6)
                  << mx[0] << "," << mx[1] << "," << mx[2] << "," << mx[3] << "," << mx[4] << ","
                  << g_burnt << "," << (g_right ? 1 : 0) << "," << g_cs << "\n";
    }
    MPI_Finalize();
    return 0;
}