#pragma once
#include "Dandelifeon.hpp"
#include "Symmetry.hpp"
#include <random>
#include <algorithm>
#include <fstream>
#include <iomanip>

struct Genome {
    Dandelifeon::Point points[24];
    int activeCount = 12, symmetryType = 0, tick = 0, initialTotal = 0;
    long mana = 0;
    double fitness = -1e18;
};

class Mutation {
public:
    static void apply(Genome& g, std::mt19937& rng, int stag) {
        bool on_plateau = (g.mana >= Dandelifeon::MANA_CAP * 0.95);
        int t = g.symmetryType;
        // Границы: 0 - ASYM (24x24), остальные - Половины (11x24)
        int mx = (t == 0) ? 24 : 11;
        int my = 24;

        // 1. ДЕЦИМАЦИЯ (Если результат хороший - режем блоки)
        if (on_plateau && rng() % 100 < 30 && g.activeCount > 3) {
            int idx = rng() % g.activeCount;
            g.points[idx] = g.points[g.activeCount - 1];
            g.activeCount--;
            return;
        }

        int mode = rng() % 100;
        
        // 2. ТЯЖЕЛАЯ МУТАЦИЯ (Сдвиги и Инверсии при застое)
        if (stag > 10000 || mode < 15) {
            int subMode = rng() % 3;
            if (subMode == 0) { // Сдвиг всей системы
                int dx = (rng() % 3) - 1, dy = (rng() % 3) - 1;
                for (int i = 0; i < g.activeCount; i++) {
                    g.points[i].x = std::clamp(g.points[i].x + dx, 0, mx);
                    g.points[i].y = std::clamp(g.points[i].y + dy, 0, my);
                }
            } else if (subMode == 1) { // Локальное зеркало (Инверсия)
                for (int i = 0; i < g.activeCount; i++) {
                    if (rng() % 2) g.points[i].x = mx - g.points[i].x;
                }
            } else { // Добавление новой точки рядом с существующей
                if (g.activeCount < 24) {
                    int pIdx = rng() % g.activeCount;
                    g.points[g.activeCount++] = {
                        std::clamp(g.points[pIdx].x + (int)(rng()%5-2), 0, mx),
                        std::clamp(g.points[pIdx].y + (int)(rng()%5-2), 0, my)
                    };
                }
            }
        } 
        else { 
            // 3. ОБЫЧНЫЙ ДРЕЙФ
            int i = rng() % g.activeCount;
            g.points[i].x = std::clamp(g.points[i].x + (int)(rng() % 3 - 1), 0, mx);
            g.points[i].y = std::clamp(g.points[i].y + (int)(rng() % 3 - 1), 0, my);
        }
    }
};

class BoardEngine {
public:
    static void evaluate(Genome& gen) {
        Dandelifeon::Board b1, b2;
        b1.clear();
        gen.initialTotal = Symmetry::apply(b1, gen.symmetryType, gen.points, gen.activeCount);

        Dandelifeon::Board* c = &b1, * n = &b2;
        gen.mana = 0; gen.fitness = -1e18;

        for (int t = 1; t <= Dandelifeon::MAX_TICKS; ++t) {
            bool wipe;
            int inC = Dandelifeon::step(*c, *n, wipe);
            if (wipe) {
                gen.tick = t;
                gen.mana = (long)inC * t * Dandelifeon::MANA_PER_GEN;
                long capped = std::min(Dandelifeon::MANA_CAP, gen.mana);
                
                if (capped >= Dandelifeon::MANA_CAP) {
                    // Фитнес для Экономии
                    gen.fitness = 1e9 + (100 - gen.initialTotal) * 1000.0 + (gen.mana - capped);
                } else {
                    // Фитнес для Роста (Кубическая зависимость от маны)
                    gen.fitness = std::pow((double)capped, 3.0) / 1e6 - (gen.initialTotal * 10.0);
                }
                return;
            }
            std::swap(c, n);
            if (t % 20 == 0) { // Проверка на смерть
                bool any = false;
                for(int i=1; i<=25; i++) if(*(uint64_t*)&c->g[i][1] || c->g[i][25]) { any=true; break; }
                if(!any) return;
            }
        }
    }
};

class FileUtil {
public:
    static void saveLeader(const Genome& gen) {
        std::ofstream f("current_leader.txt");
        if (!f.is_open()) return;
        double bpm = (gen.initialTotal * 120.0) / (gen.tick ? gen.tick : 1);
        f << "Strategy: " << Symmetry::getName(gen.symmetryType) << "\n"
          << "Mana: " << gen.mana << "\n"
          << "Initial Blocks: " << gen.initialTotal << "\n"
          << "Last Tick: " << gen.tick << "\n"
          << "Blocks per Minute: " << std::fixed << std::setprecision(1) << bpm << "\n"
          << "--------------------------\n";
        Dandelifeon::Board b; b.clear();
        Symmetry::apply(b, gen.symmetryType, gen.points, gen.activeCount);
        for (int y = 1; y <= 25; ++y) {
            for (int x = 1; x <= 25; ++x) f << (b.g[y][x] ? "C " : ". ");
            f << "\n";
        }
        f.close();
    }
};
