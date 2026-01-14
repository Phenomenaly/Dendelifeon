#pragma once
#include <algorithm>
#include <vector>

#include "Genome.hpp"

namespace Dandelifeon {
    class EvolutionManager {
    public:
        static void mutate(Genome& gen, std::mt19937& rng, const Bitboard& footprint) {

            if (gen.organCount == 0 || countLifeOrgans(gen) == 0) {
                forceAddLife(gen, rng);
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
                if (target.isObstacle) break;

                int dx = (int)(rng() % 3) - 1;
                int dy = (int)(rng() % 3) - 1;
                target.x = (int8_t)std::clamp((int)target.x + dx, 0, 24);
                target.y = (int8_t)std::clamp((int)target.y + dy, 0, 24);
            }
            break;

            case 2: // Shift one cell
            {
                if (target.isObstacle) break;

                if (target.count > 0) {
                    int p_idx = rng() % target.count;
                    int dx = (int)(rng() % 3) - 1;
                    int dy = (int)(rng() % 3) - 1;
                    target.cells[p_idx].dx = (int8_t)std::clamp((int)target.cells[p_idx].dx + dx, -5, 5);
                    target.cells[p_idx].dy = (int8_t)std::clamp((int)target.cells[p_idx].dy + dy, -5, 5);
                }
            }
            break;

            case 3: // Mirroring relative to board center
            {
                target.x = (int8_t)(24 - (int)target.x);
                target.y = (int8_t)(24 - (int)target.y);
            }
            break;

            case 4: // Mirroring relative to mass center
            {
                target.mirrorLocal(rng() % 2 == 0, rng() % 2 == 0);
            }
            break;

            case 5: // Invert one cell
            {
                bool remove = (target.count >= 10) || (target.count > 1 && rng() % 2 == 0);

                if (remove) target.count--;

                else target.addPoint((rng() % 3) - 1, (rng() % 3) - 1);

                if (target.count == 0) {
                    gen.organs[org_idx] = gen.organs[gen.organCount - 1];
                    gen.organCount--;
                }
            }
            break;

            case 6: // Toggle Symmetry
            {
                gen.symmetric = !gen.symmetric;
            }
            break;

            case 7: // invert wall
            {
                bool try_remove = (rng() % 2 == 0);

                if (try_remove) {
                    // it's far from the most optimal way, but it should be enough.
                    for (int i = 0; i < gen.organCount; ++i) {
                        if (gen.organs[i].isObstacle) {
                            gen.organs[i] = gen.organs[gen.organCount - 1];
                            gen.organCount--;
                            break;
                        }
                    }
                }
                else {
                    if (gen.organCount < 15) {
                        Structure wall;
                        wall.isObstacle = true;
                        wall.x = rng() % 25;
                        wall.y = rng() % 25;

                        wall.addPoint(0, 0);
                        gen.organs[gen.organCount++] = wall;
                    }
                }
            }
            break;

            case 8: // Smart Obstacle
            {
                if (gen.organCount >= 15) break;

                for (int k = 0; k < 20; ++k) {
                    int ty = 1 + rng() % 25;

                    if (footprint.data[ty] == 0) continue;

                    int tx = rng() % 25;
                    if (footprint.data[ty] & (1 << tx)) {
                        Structure obs;
                        obs.x = (int8_t)tx;
                        obs.y = (int8_t)(ty - 1);
                        obs.isObstacle = true;
                        obs.addPoint(0, 0);

                        gen.organs[gen.organCount++] = obs;
                        break;
                    }
                }
            }
            break;

            }

            if (countLifeOrgans(gen) == 0) forceAddLife(gen, rng);
        }

    private:
        static int countLifeOrgans(const Genome& gen) {
            int c = 0;
            for (int i = 0; i < gen.organCount; ++i) {
                if (!gen.organs[i].isObstacle) c++;
            }
            return c;
        }

        static void forceAddLife(Genome& gen, std::mt19937& rng) {
            // Usallly its not called when organCount more then 1
            if (gen.organCount >= 15) return;

            Structure new_org;
            new_org.isObstacle = false;
            new_org.x = (int8_t)(rng() % 25);
            new_org.y = (int8_t)(rng() % 25);

            // 3...4 cell 
            new_org.addPoint(0, 0);
            if (3 + rng() % 2 == 0) new_org.addPoint(1, 0);

            gen.organs[gen.organCount++] = new_org;
        }
    };
}