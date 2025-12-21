#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include "Optimizer.hpp"

std::mutex g_best_mutex, g_pool_mutex;
Genome g_global_best;
std::vector<Genome> g_pools[4];
std::vector<long> g_thread_bests;
std::vector<int> g_thread_blocks;
std::vector<int> g_thread_syms;
std::atomic<unsigned long long> g_iters{ 0 };

void monitor() {
    int n = (int)g_thread_bests.size();
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(g_best_mutex);
        std::cout << "\033[H\033[2J___LEADERBOARD (9 THREADS)___\n";
        std::cout << std::left << std::setw(12) << "Strategy" << " | " << std::setw(15) << "Real Best" << " | " << "b\n";
        std::cout << "------------------------------------\n";
        for (int i = 0; i < n; i++) {
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
    bool isAsymLab = (sym == 0);
    Genome p; p.symmetryType = sym;
    auto reset = [&]() {
        p.activeCount = (sym == 0) ? 14 : 8;
        int mx = (sym == 0) ? 24 : 11;
        for (int i = 0; i < 24; i++) p.points[i] = { (int)(rng() % (mx+1)), (int)(rng() % 25) };
        BoardEngine::evaluate(p);
    };
    reset();
    long lastImp = 0, iters = 0;
    while (true) {
        iters++; g_iters++;
        
        if (!isAsymLab && iters % 10000000 == 0) {
            std::lock_guard<std::mutex> lock(g_pool_mutex);
            int donorType = rng() % 4;
            if (donorType != sym && !g_pools[donorType].empty()) {
                const Genome& donor = g_pools[donorType][rng() % g_pools[donorType].size()];

                int cx = rng() % 20, cy = rng() % 20;
                int write = 0;
                for(int i=0; i<p.activeCount; i++) if (std::abs(p.points[i].x - cx) > 2 || std::abs(p.points[i].y - cy) > 2) p.points[write++] = p.points[i];
                for(int i=0; i<donor.activeCount; i++) if (std::abs(donor.points[i].x - cx) <= 2 && std::abs(donor.points[i].y - cy) <= 2) if(write < 24) p.points[write++] = donor.points[i];
                p.activeCount = write;
                BoardEngine::evaluate(p);
            }
        }

        Genome c = p; Mutation::apply(c, rng, (int)(iters - lastImp));
        BoardEngine::evaluate(c);
        if (c.fitness >= p.fitness || (rng() % 1000 == 0)) {
            if (c.fitness > p.fitness) lastImp = iters;
            p = c;
            std::lock_guard<std::mutex> lock(g_best_mutex);
            if (p.mana > g_thread_bests[id]) { g_thread_bests[id] = p.mana; g_thread_blocks[id] = p.initialTotal; }
            
            bool isNewGlobal = false;
            long capP = std::min(Dandelifeon::MANA_CAP, p.mana);
            long capG = std::min(Dandelifeon::MANA_CAP, g_global_best.mana);
            if (capP > capG) isNewGlobal = true;
            else if (capP == capG && p.initialTotal < (g_global_best.initialTotal ? g_global_best.initialTotal : 999)) isNewGlobal = true;

            if (isNewGlobal) {
                g_global_best = p; FileUtil::saveLeader(p);
                std::lock_guard<std::mutex> plock(g_pool_mutex);
                g_pools[sym].push_back(p); if(g_pools[sym].size() > 10) g_pools[sym].erase(g_pools[sym].begin());
            }
        }
        if (iters - lastImp > 1000000) { reset(); lastImp = iters; }
    }
}

void startCustomOptimization(int maxTicks, int manaPerTick, long manaCap, const std::vector<SymType>& strategies) {
    Dandelifeon::MAX_TICKS = maxTicks;
    Dandelifeon::MANA_PER_GEN = manaPerTick;
    Dandelifeon::MANA_CAP = 6LL * maxTicks * manaPerTick;
    int n = (int)strategies.size();
    g_thread_bests.assign(n, 0);
    g_thread_blocks.assign(n, 999);
    g_thread_syms.assign(n, 0);
    for (int i = 0; i < n; i++) g_thread_syms[i] = (int)strategies[i];
    std::vector<std::thread> ths;
    for (int i = 0; i < n; i++) ths.emplace_back(evolutionTask, i, (int)strategies[i]);
    std::thread ui(monitor); ui.detach();
    for (auto& t : ths) t.join();
}

int main() {
    #ifdef _WIN32
    std::system("cls");
    #endif
    startCustomOptimization(60, 150, 50000, {
        ASYM, ASYM, ASYM, ASYM, ASYM, 
        HR180, HR180, 
        HMX, HMX });
    return 0;
}

