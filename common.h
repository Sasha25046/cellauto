
#pragma once
#include <cstdint>

inline uint64_t hash_coords(uint64_t seed, uint64_t i, uint64_t j) {
    uint64_t h = seed ^ (i * 0x517cc1b727220a95ULL) ^ (j * 0x9e3779b97f4a7c15ULL);
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return h;
}

inline bool is_tree(uint64_t seed, int i, int j, double d) {
    uint64_t val = hash_coords(seed, (uint64_t)i, (uint64_t)j);
    double prob = (double)val / (double)0xFFFFFFFFFFFFFFFFULL;
    return prob < d;
}

inline uint64_t cell_checksum(int glob_i, int j, uint8_t state) {
    return hash_coords(0xC0FFEE00ULL + state, (uint64_t)glob_i, (uint64_t)j);
}