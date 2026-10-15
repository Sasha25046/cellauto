#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <string>
#include "common.h"

int main(int argc, char* argv[]) {
    int N = 1000;
    double d = 0.60;
    uint64_t seed = 42ULL;
    if (argc >= 2) N = std::stoi(argv[1]);
    if (argc >= 3) d = std::stod(argv[2]);
    if (argc >= 4) seed = std::stoull(argv[3]);

    std::vector<uint8_t> cur((size_t)N * N, 0), nxt((size_t)N * N, 0);

    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            if (is_tree(seed, i, j, d))
                cur[(size_t)i * N + j] = (j == 0) ? 2 : 1;

    auto t0 = std::chrono::high_resolution_clock::now();

    int step = 0;
    bool reached_right = false;
    long total_burnt = 0;

    while (true) {
        long burning_count = 0;
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                uint8_t state = cur[(size_t)i * N + j];
                uint8_t ns = state;
                if (state == 2) {
                    ns = 3;
                    total_burnt++;
                    if (j == N - 1) reached_right = true;
                } else if (state == 1) {
                    bool nb = false;
                    if (i > 0     && cur[(size_t)(i - 1) * N + j] == 2) nb = true;
                    if (i < N - 1 && cur[(size_t)(i + 1) * N + j] == 2) nb = true;
                    if (j > 0     && cur[(size_t)i * N + (j - 1)] == 2) nb = true;
                    if (j < N - 1 && cur[(size_t)i * N + (j + 1)] == 2) nb = true;
                    if (nb) { ns = 2; burning_count++; }
                }
                nxt[(size_t)i * N + j] = ns;
            }
        }
        cur.swap(nxt);
        step++;
        if (burning_count == 0) break;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t1 - t0).count();

    uint64_t checksum = 0;
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            checksum += cell_checksum(i, j, cur[(size_t)i * N + j]);

    std::cout << "=== V0 (Sequential) Finished ===\n";
    std::cout << "Grid size N: " << N << " x " << N << "\n";
    std::cout << "Density d: " << d << "\n";
    std::cout << "Seed: " << seed << "\n";
    std::cout << "Steps: " << step << "\n";
    std::cout << "Total burnt cells: " << total_burnt << "\n";
    std::cout << "Reached right edge: " << (reached_right ? "YES" : "NO") << "\n";
    std::cout << "Checksum: " << checksum << "\n";
    std::cout << "Execution time: " << std::fixed << std::setprecision(4) << elapsed << " seconds\n";
    std::cout << "CSV,1," << N << "," << step << "," << std::setprecision(6) << elapsed << "," << elapsed
              << ",0,0,0," << total_burnt << "," << (reached_right ? 1 : 0) << "," << checksum << "\n";
    return 0;
}