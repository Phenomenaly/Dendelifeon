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

class FileUtil {
public:
    static void saveLeader(const Genome& gen) {
        std::ofstream f("current_leader.txt");
        if (!f.is_open()) return;

        double bpm = (gen.initialTotal * 120.0) / (gen.tick ? gen.tick : 1);

        f << "Symmetry: " << Symmetry::getName(gen.symmetryType) << "\n";
        f << "Mana: " << gen.mana << "\n";
        f << "Initial Blocks: " << gen.initialTotal << "\n";
        f << "Last Tick: " << gen.tick << "\n";
        f << "Blocks per Minute: " << std::fixed << std::setprecision(1) << bpm << "\n";
        f << "--------------------------\n";

        Dandelifeon::Board b; b.clear();
        Symmetry::apply(b, gen.symmetryType, gen.points, gen.activeCount);
        for (int y = 1; y <= 25; ++y) {
            for (int x = 1; x <= 25; ++x) f << (b.g[y][x] ? "C " : ". ");
            f << "\n";
        }
        f.close();
    }
};

class BoardEngine {
public:
    static void evaluate(Genome& gen) {
        Dandelifeon::Board b1, b2; b1.clear();
        gen.initialTotal = Symmetry::apply(b1, gen.symmetryType, gen.points, gen.activeCount);

        Dandelifeon::Board* c = &b1, * n = &b2;
        for (int t = 1; t <= Dandelifeon::MAX_TICKS; ++t) {
            bool wipe;
            int inC = Dandelifeon::step(*c, *n, wipe);
            if (wipe) {
                gen.tick = t;
                gen.mana = (long)inC * t * Dandelifeon::MANA_PER_GEN;
                long capped = std::min(Dandelifeon::MANA_CAP, gen.mana);
                if (capped >= Dandelifeon::MANA_CAP) gen.fitness = 1e9 + (100 - gen.initialTotal) * 1000.0;
                else gen.fitness = std::pow((double)capped, 2.0) / 1000.0 - (gen.initialTotal * 5.0);
                return;
            }
            std::swap(c, n);
        }
        gen.fitness = -1e12;
    }
};

class Mutation {
public:
    static void apply(Genome& g, std::mt19937& rng, int stag) {
        int mx = (g.symmetryType == ASYM) ? 24 : 11;
        int my = 24;
        int mode = rng() % 100;

        if (mode < 12 && g.activeCount > 3) {
            g.points[rng() % g.activeCount] = g.points[--g.activeCount];
        }
        else if (mode < 25 && g.activeCount < 24) {
            int pIdx = rng() % g.activeCount;
            g.points[g.activeCount++] = {
                std::clamp(g.points[pIdx].x + (int)(rng() % 5 - 2), 0, mx),
                std::clamp(g.points[pIdx].y + (int)(rng() % 5 - 2), 0, my)
            };
        }
        else {
            int i = rng() % g.activeCount;
            int d = (stag > 20000) ? 5 : 1;
            g.points[i].x = std::clamp(g.points[i].x + (int)(rng() % (d * 2 + 1) - d), 0, mx);
            g.points[i].y = std::clamp(g.points[i].y + (int)(rng() % (d * 2 + 1) - d), 0, my);
        }
    }
};
