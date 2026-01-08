#pragma once
#include <vector>
#include <array>
#include <random>

#include "Structure.hpp"
#include "DandelifeonEngine.hpp"


namespace Dandelifeon {
    struct Genome {
        Structure organs[5];
        int8_t organCount = 0;

        bool symmetric = false;

        std::array<double, 7> mutationWeights;
        int lastMutationType = -1;

        Genome() {
            mutationWeights.fill(1.0 / 7.0);
            organCount = 0;
            symmetric = false
        }

        int selectMutation(std::mt19937& rng) {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double r = dist(rng);
            double cumulative = 0;
            for (int i = 0; i < 7; ++i) {
                cumulative += mutationWeights[i];
                if (r <= cumulative) 
                    return i;
            }
            return 6;
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

        std::pair<Bitboard, Bitboard> toBitboards() const {
            Bitboard cells; cells.clear();
            Bitboard obs;   obs.clear();

            for (int i = 0; i < organCount; ++i) {
                const auto& org = organs[i];
                for (int j = 0; j < org.count; ++j) {
                    int realX = org.x + org.cells[j].dx;
                    int realY = org.y + org.cells[j].dy;

                    if (realX >= 0 && realX < 25 && realY >= 0 && realY < 25) {
                        uint32_t bit = (1 << realX);
                        if (org.isObstacle) {
                            obs.data[realY + 1] |= bit;
                            if (symmetric) {
                                obs.data[(24 - realY) + 1] |= (1 << (24 - realX));
                            }
                        } 
                        else {
                            cells.data[realY + 1] |= bit;
                            if (symmetric) {
                                cells.data[(24 - realY) + 1] |= (1 << (24 - realX));
                            }
                        }
                    }
                }
            }
            return { cells, obs };
        }
    };

}


