#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "Optimizer.hpp"

std::mutex g_best_mutex;
Genome g_global_best;
std::vector<long> g_thread_bests;
std::vector<int> g_thread_blocks;
std::vector<int> g_thread_syms;
std::atomic<unsigned long long> g_iters{ 0 };

void monitor() {
    int num_threads = g_thread_bests.size();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(g_best_mutex);

        std::cout << "\033[H\033[2J___LEADERBOARD___\n";
        std::cout << std::left << std::setw(12) << "Strategy" << " | " << std::setw(15) << "Best result" << " | " << "b\n";
        std::cout << "------------------------------------\n";

        for (int i = 0; i < num_threads; i++) {
            std::cout << std::left << std::setw(12) << Symmetry::getName(g_thread_syms[i]) << " | "
                << std::setw(15) << g_thread_bests[i] << " | " << g_thread_blocks[i] << "b\n";
        }

        std::cout << "~~~~~~~~~~~~~~~\n";
        if (g_global_best.mana > 0) {
            double bpm = (g_global_best.initialTotal * 120.0) / (g_global_best.tick ? g_global_best.tick : 1);
            std::cout << "Leader: " << Symmetry::getName(g_global_best.symmetryType) << " | "
                << g_global_best.mana << " | " << g_global_best.initialTotal << "b ("
                << std::fixed << std::setprecision(1) << bpm << "b/min) | " << g_global_best.tick << "t\n";
        }
        std::cout << "\nTotal Progress: " << std::fixed << std::setprecision(2) << (g_iters.load() / 1000000.0) << "M iterations\n";
    }
}

void evolutionTask(int id, int sym) {
    std::mt19937 rng(static_cast<unsigned>(std::time(0)) + id);
    Genome p; p.symmetryType = sym;
    auto reset = [&]() {
        p.activeCount = (sym == ASYM) ? 14 : 8;
        for (int i = 0; i < 24; i++) p.points[i] = { (int)(rng() % 25), (int)(rng() % 25) };
        BoardEngine::evaluate(p);
        };
    reset();
    long lastImp = 0, iters = 0;
    while (true) {
        iters++; g_iters++;
        Genome c = p; Mutation::apply(c, rng, iters - lastImp);
        BoardEngine::evaluate(c);
        if (c.fitness >= p.fitness) {
            if (c.fitness > p.fitness) lastImp = iters;
            p = c;
            std::lock_guard<std::mutex> lock(g_best_mutex);
            if (p.mana > g_thread_bests[id]) { g_thread_bests[id] = p.mana; g_thread_blocks[id] = p.initialTotal; }

            bool isNewGlobal = false;
            long cappedP = std::min(Dandelifeon::MANA_CAP, p.mana);
            long cappedG = std::min(Dandelifeon::MANA_CAP, g_global_best.mana);
            if (cappedP > cappedG) isNewGlobal = true;
            else if (cappedP == cappedG && p.initialTotal < (g_global_best.initialTotal ? g_global_best.initialTotal : 999)) isNewGlobal = true;

            if (isNewGlobal) { g_global_best = p; FileUtil::saveLeader(p); }
        }
        if (iters - lastImp > 500000) { reset(); lastImp = iters; }
    }
}

void startOptimization(int maxTicks, int manaPerTick, long manaCap, const std::vector<SymType>& strategies) {
    Dandelifeon::MAX_TICKS = maxTicks;
    Dandelifeon::MANA_PER_GEN = manaPerTick;
    Dandelifeon::MANA_CAP = manaCap;

    int n = strategies.size();
    g_thread_bests.assign(n, 0);
    g_thread_blocks.assign(n, 0);
    g_thread_syms.assign(n, 0);
    for (int i = 0; i < n; i++) g_thread_syms[i] = strategies[i];

    std::vector<std::thread> ths;
    for (int i = 0; i < n; i++) {
        ths.emplace_back(evolutionTask, i, strategies[i]);
    }
    std::thread ui(monitor);
    ui.detach();

    for (auto& t : ths) t.join();
}

int main() {
#ifdef _WIN32
    std::system("cls");
#endif

    startOptimization(
        60,
        150,
        50000,
        { ASYM, HR180, HMX, DIAG }
    );

    return 0;
}
