#pragma once
#define NOMINMAX
#include <cstdint>
#include <array>
#include <cstring>
#include <immintrin.h>
#include <intrin.h>
#include <algorithm>
#include <cmath>


namespace Dandelifeon {
    struct alignas(32) Bitboard {
        uint32_t data[34];

        void clear() { std::memset(data, 0, sizeof(data)); }

        bool isEmpty() const {
            for (int i = 1; i <= 25; ++i) if (data[i]) 
                return false;

            return true;
        }

        uint32_t& operator[](size_t i) { return data[i]; }
        const uint32_t& operator[](size_t i) const { return data[i]; }
    };

    struct SimulationResult {
        long mana = 0;
        double fitness = 0;
        int ticks = 0;
        int initial_blocks = 0;
        double pheno_x = 0;
        double pheno_y = 0;
        bool success = false;
    };

    class Engine {
    public:
        int max_ticks; int mana_per_gen; long mana_cap;
        __m256i row_mask;

        Engine(int mt = 100, int mpg = 60, long mc = 50000)
            : max_ticks(mt), mana_per_gen(mpg), mana_cap(mc) {
            row_mask = _mm256_set1_epi32(0x1FFFFFF);
        }

        // Pure black magic of bitwise operations and overtaking several lines at once
        inline void step_avx2(const Bitboard& current, Bitboard& next) const {
            for (int i = 1; i <= 25; i += 8) {
                __m256i mid = _mm256_loadu_si256((const __m256i*) & current.data[i]);
                __m256i top = _mm256_loadu_si256((const __m256i*) & current.data[i - 1]);
                __m256i bot = _mm256_loadu_si256((const __m256i*) & current.data[i + 1]);

                __m256i n1 = _mm256_slli_epi32(top, 1);  __m256i n2 = top; __m256i n3 = _mm256_srli_epi32(top, 1);
                __m256i n4 = _mm256_slli_epi32(mid, 1);                   __m256i n5 = _mm256_srli_epi32(mid, 1);
                __m256i n6 = _mm256_slli_epi32(bot, 1);  __m256i n7 = bot; __m256i n8 = _mm256_srli_epi32(bot, 1);

                __m256i s0 = _mm256_setzero_si256(), s1 = _mm256_setzero_si256(), s2 = _mm256_setzero_si256();

                auto add = [&](__m256i x) {
                    __m256i c0 = _mm256_and_si256(s0, x); s0 = _mm256_xor_si256(s0, x);
                    __m256i c1 = _mm256_and_si256(s1, c0); s1 = _mm256_xor_si256(s1, c0); s2 = _mm256_or_si256(s2, c1);
                    };

                add(n1); add(n2); add(n3); add(n4); add(n5); add(n6); add(n7); add(n8);

                __m256i res = _mm256_and_si256(_mm256_andnot_si256(s2, s1), _mm256_or_si256(s0, mid));
                _mm256_storeu_si256((__m256i*) & next.data[i], _mm256_and_si256(res, row_mask));
            }
        }

        void getPhenotype(const Bitboard& b, double& x_out, double& y_out) const {
            int total = 0; int x_min = 25, x_max = 0, y_min = 25, y_max = 0;
            double sum_dist = 0; int count_structs = 0;
            for (int y = 1; y <= 25; ++y) {
                uint32_t row = b.data[y];
                if (row) {
                    total += __popcnt(row);
                    y_min = (std::min)(y_min, y); y_max = (std::max)(y_max, y);
                    unsigned long first, last;
                    _BitScanForward(&first, row); _BitScanReverse(&last, row);
                    x_min = (std::min)(x_min, (int)first); x_max = (std::max)(x_max, (int)last);
                    sum_dist += std::abs(y - 13);
                    count_structs++;
                }
            }
            if (total == 0) { 
                x_out = 0; y_out = 0; 
                return; 
            }

            x_out = (double)total / ((x_max - x_min + 1) * (y_max - y_min + 1));
            y_out = sum_dist / count_structs;
        }

        SimulationResult run(const Bitboard& start_board) const {
            SimulationResult res;
            res.initial_blocks = 0;
            for (int i = 1; i <= 25; ++i) 
                res.initial_blocks += __popcnt(start_board.data[i]);

            Bitboard buffer, curr_b = start_board;
            Bitboard* curr = &curr_b, * nxt = &buffer;
            uint32_t center_mask = (1 << 11) | (1 << 12) | (1 << 13);

            double best_proximity = 0;

            for (int t = 1; t <= max_ticks; ++t) {
                nxt->clear();
                step_avx2(*curr, *nxt);

                uint32_t hits = ((*nxt)[12] | (*nxt)[13] | (*nxt)[14]) & center_mask;
                if (hits) {
                    int cells = __popcnt((*nxt)[12] & center_mask) + __popcnt((*nxt)[13] & center_mask) + __popcnt((*nxt)[14] & center_mask);
                    
                    res.mana = (std::min)(mana_cap, (long)cells * t * mana_per_gen);
                    res.ticks = t;
                    res.success = true;

                    // More then 4 blocks have way more value
                    double multiplier = 0.5;
                    if (cells > 3) multiplier = 1.0;
                    if (cells > 4) multiplier = 2.0;

                    res.fitness = (double)res.mana * multiplier + t * 50;
                    
                    return res;
                }

                for (int y = 1; y <= 25; ++y) {
                    if ((*nxt)[y]) {
                        double dist_score = 25.0 - std::abs(y - 13);
                        best_proximity = (std::max)(best_proximity, dist_score + (t * 0.1));
                    }
                }

                std::swap(curr, nxt);
                if (curr->isEmpty()) break;
            }
            res.fitness = best_proximity;
            return res;
        }
    };
}

