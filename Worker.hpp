#pragma once
#include <atomic>
#include <memory>

#include "Genome.hpp"
#include "DandelifeonEngine.hpp"
#include "Archive.hpp"
#include "EvolutionManager.hpp"


namespace Dandelifeon {
    inline std::unique_ptr<std::atomic<long>[]> g_thread_mana;
    inline std::unique_ptr<std::atomic<int>[]> g_thread_blocks;
    inline std::atomic<uint64_t> g_total_iters{ 0 };

    void workerTask(int id, Archive& archive, const Engine& engine) {
        std::mt19937 rng(std::random_device{}() + id);
        Genome current_gen;

        // 3 structures with 8 cells each, should be moved to separate variables
        for (int i = 0; i < 3; i++) {
            Structure s;
            s.x = 8 + rng() % 10;
            s.y = 8 + rng() % 10;

            for (int j = 0; j < 8; j++) 
                s.addPoint(j % 3, j / 3);

            current_gen.organs[current_gen.organCount++] = s;
        }

        auto best_res = engine.run(current_gen.toBitboard());
        uint64_t local_iters = 0;
        uint64_t last_improvement = 0;

        while (true) {
            local_iters++;
            g_total_iters.fetch_add(1, std::memory_order_relaxed);

            Genome next_gen = current_gen;

            int stagnation = (int)(local_iters - last_improvement);
            int mutation_count = 1;

            if (stagnation > 500'000) 
                mutation_count = 3;
            if (stagnation > 5'000'000)
                mutation_count = 10;

            for (int i = 0; i < mutation_count; ++i) {
                EvolutionManager::mutate(next_gen, rng);
            }

            auto res = engine.run(next_gen.toBitboard());

            if (res.fitness > best_res.fitness) {
                current_gen = next_gen;
                best_res = res;
                last_improvement = local_iters;
                current_gen.rewardLastMutation();

                g_thread_mana[id].store(res.mana);
                g_thread_blocks[id].store(res.initial_blocks);

                if (res.fitness > 10.0) {
                    archive.submit(current_gen, res);
                }
            }

            if (stagnation > 100'000'000) {
                if (archive.getElite(current_gen, rng)) {
                    best_res = engine.run(current_gen.toBitboard());
                    last_improvement = local_iters;
                }
                else {
                    current_gen = Genome();
                }
            }
        }
    }
}