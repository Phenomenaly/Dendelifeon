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
        // Мы считаем, что попали в цель, если мана в пределах +- 2.5% от капа
        bool target_hit = (g.mana >= Dandelifeon::MANA_CAP * 0.975 && g.mana <= Dandelifeon::MANA_CAP * 1.025);
        int mx = (g.symmetryType == 0) ? 24 : 11;

        if (target_hit && (rng() % 100 < 40) && g.activeCount > 3) {
            int idx = rng() % g.activeCount;
            g.points[idx] = g.points[g.activeCount - 1];
            g.activeCount--;
            return;
        }

        int mode = rng() % 100;
        if (stag > 5000 || mode < 12) {
            int dx = (rng() % 3) - 1, dy = (rng() % 3) - 1;
            for (int i = 0; i < g.activeCount; i++) {
                g.points[i].x = std::clamp(g.points[i].x + dx, 0, mx);
                g.points[i].y = std::clamp(g.points[i].y + dy, 0, 24);
            }
        } else {
            int i = rng() % g.activeCount;
            g.points[i].x = std::clamp(g.points[i].x + (int)(rng() % 3 - 1), 0, mx);
            g.points[i].y = std::clamp(g.points[i].y + (int)(rng() % 3 - 1), 0, 24);
        }
        if (rng() % 100 < 5 && g.activeCount < 24) {
            g.points[g.activeCount++] = { (int)(rng() % (mx + 1)), (int)(rng() % 25) };
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
                long rawMana = (long)inC * t * Dandelifeon::MANA_PER_GEN;
                gen.mana = rawMana;

                // ПРОВЕРКА НА ПЕРЕПОЛНЕНИЕ (Буфер + 2.5%)
                if (rawMana > Dandelifeon::MANA_CAP * 1.025) {
                    gen.fitness = -1e9; // Штраф за бесполезную избыточность
                } 
                else if (rawMana >= Dandelifeon::MANA_CAP * 0.975) {
                    // ИДЕАЛЬНАЯ ЗОНА: Режим экстремальной экономии
                    gen.fitness = 2000000.0 + (100 - gen.initialTotal) * 1000.0;
                }
                else {
                    // ЗОНА РОСТА: Чем ближе к капу, тем лучше
                    gen.fitness = (double)rawMana - (gen.initialTotal * 2.0);
                }
                return;
            }
            std::swap(c, n);
            if (t % 25 == 0) {
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
        f << "Strategy: " << Symmetry::getName(gen.symmetryType) << "\nMana: " << gen.mana 
          << "\nBlocks: " << gen.initialTotal << "\nTick: " << gen.tick << "\nBPM: " << bpm << "\n---\n";
        Dandelifeon::Board b; b.clear();
        Symmetry::apply(b, gen.symmetryType, gen.points, gen.activeCount);
        for (int y = 1; y <= 25; ++y) {
            for (int x = 1; x <= 25; ++x) f << (b.g[y][x] ? "C " : ". ");
            f << "\n";
        }
    }
};


