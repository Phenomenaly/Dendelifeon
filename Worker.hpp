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

    current_gen.symmetric = (rng() % 2 == 0);

    // 1 - 2 structures with 3-7 cells each
    for (int i = 0; i < 1 + rng() % 2; i++) {
        Structure s;
        s.x = 8 + rng() % 10;
        s.y = 8 + rng() % 10;

        for (int j = 0; j < 3 + rng() % 5; j++)
            s.addPoint(j % 3, j / 3);

        current_gen.organs[current_gen.organCount++] = s;
    }

    auto init_layers = current_gen.toBitboard();
    auto best_res = engine.run(init_layers.first, init_layers.second);
    uint64_t local_iters = 0;
    uint64_t last_improvement = 0;

    while (true) {
        local_iters++;
        g_total_iters.fetch_add(1, std::memory_order_relaxed);

        Genome next_gen = current_gen;

        int stagnation = (int)(local_iters - last_improvement);
        int mutation_count = 1;

        if (stagnation > 500'000) 
            mutation_count = 5;
        if (stagnation > 5'000'000)
            mutation_count = 20;

        for (int i = 0; i < mutation_count; ++i) {
            EvolutionManager::mutate(next_gen, rng);
        }

        auto next_layers = next_gen.toBitboard();
        auto res = engine.run(next_layers.first, next_layers.second);

        if (res.fitness > best_res.fitness) {
            current_gen = next_gen;
            best_res = res;
            last_improvement = local_iters;
            current_gen.rewardLastMutation();

            g_thread_mana[id].store(res.mana);
            g_thread_blocks[id].store(res.initial_blocks);

            if (res.fitness > 10.0) {
                engine.getPhenotype(next_layers.first, res.pheno_x, res.pheno_y);
                archive.submit(current_gen, res);
            }
        }

        if (stagnation > 200'000'000) {
            if (archive.getElite(current_gen, rng)) {
                auto elite_layers = current_gen.toBitboard();
                best_res = engine.run(elite_layers.first, elite_layers.second);
                last_improvement = local_iters;
            }
            else {
                current_gen = Genome();
            }
        }
    }
}

}

