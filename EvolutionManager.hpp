#pragma once
#include <algorithm>

#include "Genome.hpp"


namespace Dandelifeon {
    class EvolutionManager {
    public:
        static void mutate(Genome& gen, std::mt19937& rng) {
            if (gen.organCount == 0) {
                forceAddStructure(gen, rng);
                return;
            }

            int type = gen.selectMutation(rng);
            gen.lastMutationType = type;

            std::uniform_int_distribution<int> organ_dist(0, gen.organCount - 1);
            int org_idx = organ_dist(rng);
            Structure& target = gen.organs[org_idx];

            switch (type) {
            case 0: // Shift board
            {
                int dx = (int)(rng() % 3) - 1;
                int dy = (int)(rng() % 3) - 1;
                for (int i = 0; i < gen.organCount; ++i) {
                    gen.organs[i].x = (int8_t)std::clamp((int)gen.organs[i].x + dx, 0, 24);
                    gen.organs[i].y = (int8_t)std::clamp((int)gen.organs[i].y + dy, 0, 24);
                }
            }
            break;

            case 1: // Shift structure
            {
                int dx = (int)(rng() % 3) - 1;
                int dy = (int)(rng() % 3) - 1;
                target.x = (int8_t)std::clamp((int)target.x + dx, 0, 24);
                target.y = (int8_t)std::clamp((int)target.y + dy, 0, 24);
            }
            break;

            case 2: // Shift one cell in the structure
            {
                if (target.count > 0) {
                    int p_idx = rng() % target.count;
                    int dx = (int)(rng() % 3) - 1;
                    int dy = (int)(rng() % 3) - 1;
                    target.cells[p_idx].dx = (int8_t)std::clamp((int)target.cells[p_idx].dx + dx, -5, 5);
                    target.cells[p_idx].dy = (int8_t)std::clamp((int)target.cells[p_idx].dy + dy, -5, 5);
                }
            }
            break;

            case 3: // Mirroring relative to the center of the board
            {
                target.x = (int8_t)(24 - (int)target.x);
                target.y = (int8_t)(24 - (int)target.y);
            }
            break;

            case 4: // Mirroring about the center of mass
            {
                target.mirrorLocal(rng() % 2 == 0, rng() % 2 == 0);
            }
            break;

            case 5: // Inver one cell
            {
                if (target.count > 1) {
                    target.count--;
                }
                else {
                    gen.organs[org_idx] = gen.organs[gen.organCount - 1];
                    gen.organCount--;
                }
            }
            break;
                
            case 6: // Symmetry
            {
                gen.symmetric = !gen.symmetric;
            }
            break;
            }

            if (gen.organCount < 2) forceAddStructure(gen, rng);
        }

    private:
        static void forceAddStructure(Genome& gen, std::mt19937& rng) {
            if (gen.organCount >= 5) return;
            Structure new_org;
            new_org.x = (int8_t)(rng() % 25);
            new_org.y = (int8_t)(rng() % 25);
            new_org.addPoint(0, 0);
            new_org.addPoint(0, 1);
            new_org.addPoint(1, 0);
            gen.organs[gen.organCount++] = new_org;
        }
    };

}
