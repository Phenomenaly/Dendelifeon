#pragma once
#include <mutex>
#include <array>
#include <vector>
#include <fstream>
#include <algorithm>
#include <random>

#include "Genome.hpp"
#include "DandelifeonEngine.hpp"


namespace Dandelifeon {
    struct ArchiveCell {
        Genome genome;
        long mana = -1;
        int blocks = 999;
        int usage_count = 0;
        bool occupied = false;
    };

    class Archive {
    private:
        std::array<std::array<ArchiveCell, 20>, 20> grid;
        std::vector<std::pair<int, int>> occupied_indices;
        std::mutex archive_mtx;

        long global_best_mana = 0;
        int global_best_blocks = 999;

        void saveToDisk(const Genome& gen, const SimulationResult& res, int ix, int iy) {
            std::ofstream f("absolute_leader.txt");
            if (!f.is_open()) return;

            f << "=== DANDELIFEON ABSOLUTE LEADER ===\n";
            f << "Mana Score: " << res.mana << "\n";
            f << "Life Blocks: " << res.initial_blocks << "\n";
            f << "Fitness (Mana/Block): " << res.fitness << "\n";
            f << "Archive Position: [X:" << ix << ", Y:" << iy << "]\n";
            f << "Symmetric: " << (gen.symmetric ? "YES" : "NO") << "\n";
            f << "-----------------------------------\n";

            Bitboard life = gen.getLifeBoard();
            Bitboard walls = gen.getObstaclesBoard();

            for (int y = 1; y <= 25; ++y) {
                for (int x = 1; x <= 25; ++x) {

                    bool is_flower = (x == 13 && y == 13);
                    bool is_life = (life.data[y] & (1 << (x - 1)));
                    bool is_wall = (walls.data[y] & (1 << (x - 1)));

                    if (is_flower)      f << "F ";
                    else if (is_wall)   f << "W ";
                    else if (is_life)   f << "C ";
                    else                f << ". ";
                }
                f << "\n";
            }
            f.close();
        }

    public:
        void submit(const Genome& gen, const SimulationResult& res) {
            // X: Density (0.0 ... 1.0) -> (0 ... 19)
            // Y: Distance (0.0 ... 18.0) -> (0 ... 19)
            int ix = std::clamp((int)(res.pheno_x * 20), 0, 19);
            int iy = std::clamp((int)((res.pheno_y / 18.0) * 20), 0, 19);

            std::lock_guard<std::mutex> lock(archive_mtx);
            ArchiveCell& cell = grid[ix][iy];

            bool is_global_record = false;
            // Mana -> blocks. But now fitness-function search mana per blocks and this is not relevant
            if (res.mana > global_best_mana) {
                global_best_mana = res.mana;
                global_best_blocks = res.initial_blocks;
                is_global_record = true;
            }
            else if (res.mana == global_best_mana && res.initial_blocks < global_best_blocks) {
                global_best_blocks = res.initial_blocks;
                is_global_record = true;
            }

            bool replace_in_cell = false;
            if (!cell.occupied || cell.usage_count >= 3) {
                replace_in_cell = true;
            }
            else {
                if (res.mana > cell.mana)
                    replace_in_cell = true;
                else if (res.mana == cell.mana && res.initial_blocks < cell.blocks)
                    replace_in_cell = true;
            }

            if (replace_in_cell) {
                if (!cell.occupied)
                    occupied_indices.push_back({ ix, iy });

                cell.genome = gen;
                cell.mana = res.mana;
                cell.blocks = res.initial_blocks;
                cell.usage_count = 0;
                cell.occupied = true;
            }

            if (is_global_record) {
                saveToDisk(gen, res, ix, iy);
            }
        }

        bool getElite(Genome& out_gen, std::mt19937& rng) {
            std::lock_guard<std::mutex> lock(archive_mtx);

            if (occupied_indices.empty())
                return false;

            auto pos = occupied_indices[rng() % occupied_indices.size()];
            auto& cell = grid[pos.first][pos.second];

            for (int i = 0; i < 9; i++) {
                out_gen.mutationWeights[i] = (out_gen.mutationWeights[i] + cell.genome.mutationWeights[i]) / 2.0;
            }
            
            for (int i = 0; i < 15; i++) {
                out_gen.organs[i] = cell.genome.organs[i];
            }
            out_gen.organCount = cell.genome.organCount;

            out_gen.symmetric = cell.genome.symmetric;

            cell.usage_count++;
            return true;
        }
    };
}