#pragma once
#include <iostream>
#include <vector>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <chrono>


namespace Dandelifeon {
    class Leaderboard {
    private:
        int num_threads;
        long global_max_mana = 0;
        int global_min_blocks = 999;
        std::mutex leaderboard_mtx;
        std::chrono::steady_clock::time_point last_time;
        uint64_t last_iters = 0;
        double current_speed = 0;

    public:
        Leaderboard(int n) : num_threads(n) {
            last_time = std::chrono::steady_clock::now();
        }

        void updateGlobal(long mana, int blocks) {
            std::lock_guard<std::mutex> lock(leaderboard_mtx);
            if (mana > global_max_mana) {
                global_max_mana = mana;
                global_min_blocks = blocks;
            }
            else if (mana == global_max_mana && blocks < global_min_blocks) {
                global_min_blocks = blocks;
            }
        }

        void draw(const std::vector<long>& thread_mana,
            const std::vector<int>& thread_blocks,
            uint64_t total_iters) {

            // (M iters per s)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count();
            if (elapsed >= 1000) {
                current_speed = (double)(total_iters - last_iters) / (elapsed * 1000.0); // M iters/s
                last_iters = total_iters;
                last_time = now;
            }

            for (int i = 0; i < num_threads; ++i) {
                if (thread_mana[i] > 0)
                    updateGlobal(thread_mana[i], thread_blocks[i]);
            }

            std::stringstream ss;

            ss << "\033[H";

            ss << "===== DANDELIFEON MONITOR =====\n";
            ss << std::left << std::setw(6) << "ID"
                << std::setw(12) << "Mana"
                << "Initial Blocks\n";
            ss << "-------------------------------------------\n";

            int display_limit = (num_threads > 15) ? 15 : num_threads;

            for (int i = 0; i < display_limit; ++i) {
                ss << "T" << std::left << std::setw(5) << i
                    << std::setw(12) << thread_mana[i]
                    << thread_blocks[i] << "      \n";
            }

            if (num_threads > display_limit) {
                ss << "... and " << (num_threads - display_limit) << " more threads processing ...\n";
            }

            ss << "-------------------------------------------\n";

            {
                std::lock_guard<std::mutex> lock(leaderboard_mtx);
                ss << "GLOBAL BEST: " << std::setw(6) << global_max_mana << " mana | "
                    << global_min_blocks << " blocks\n";
            }

            ss << "TOTAL PROGRESS: " << std::fixed << std::setprecision(2) << (total_iters / 1000000.0) << " M simulation\n";
            ss << "CURRENT SPEED:  " << std::fixed << std::setprecision(2) << current_speed << " M simulation/s\n";

            ss << "\033[J";

            std::cout << ss.str() << std::flush;
        }
    };
}