#pragma once
#include <vector>
#include <array>
#include <random>

#include "Structure.hpp"
#include "DandelifeonEngine.hpp"


namespace Dandelifeon {
    struct Genome {
        Structure organs[15];
        int8_t organCount = 0;

        bool symmetric = false;

        std::array<double, 9> mutationWeights;
        int lastMutationType = -1;

        Genome() {
            mutationWeights.fill(1.0 / 9.0);
            // Add "smart" wall is way more valuable
            mutationWeights[8] = 0.4;
            organCount = 0;
            symmetric = true;
        }

        int selectMutation(std::mt19937& rng) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double r = dist(rng);
            double cumulative = 0;
            for (int i = 0; i < 9; ++i) {
                cumulative += mutationWeights[i];
                if (r <= cumulative)  return i;
            }

            return 8;
        }

        void rewardLastMutation() {
            if (lastMutationType == -1) 
                return;

            double boost = 0.05;
            mutationWeights[lastMutationType] += boost;
            double sum = 0;

            for (double w : mutationWeights) 
                sum += w;

            for (double& w : mutationWeights) 
                w /= sum;
        }

        void drawOrgan(const Structure& org, Bitboard& b) const {
            for (int j = 0; j < org.count; ++j) {
                int realX = org.x + org.cells[j].dx;
                int realY = org.y + org.cells[j].dy;

                if (realX >= 0 && realX < 25 && realY >= 0 && realY < 25) {
                    b.data[realY + 1] |= (1 << realX);

                    if (symmetric) {
                        int symX = 24 - realX;
                        int symY = 24 - realY;
                        b.data[symY + 1] |= (1 << symX);
                    }
                }
            }
        }

        Bitboard getLifeBoard() const {
            Bitboard b; b.clear();
            for (int i = 0; i < organCount; ++i) {
                // Рисуем, только если это НЕ стена
                if (!organs[i].isObstacle) {
                    drawOrgan(organs[i], b);
                }
            }
            return b;
        }

        Bitboard getObstaclesBoard() const {
            Bitboard b; b.clear();
            for (int i = 0; i < organCount; ++i) {
                if (organs[i].isObstacle) {
                    drawOrgan(organs[i], b);
                }
            }

            uint32_t center_mask = (1 << 11) | (1 << 12) | (1 << 13); // 0x1C00 = (5 << 11)
            b.data[12] &= ~center_mask;
            b.data[13] &= ~center_mask;
            b.data[14] &= ~center_mask;

            return b;
        }
    };
}