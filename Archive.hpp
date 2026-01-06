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
        // 20 seems too much to me, a number between 8 and 16 is better
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
            f << "Initial Blocks: " << res.initial_blocks << "\n";
            f << "Ticks to Center: " << res.ticks << "\n";
            f << "Archive Position: [X:" << ix << ", Y:" << iy << "]\n";
            f << "Phenotype X (Density): " << res.pheno_x << "\n";
            f << "Phenotype Y (Distance): " << res.pheno_y << "\n";
            f << "-----------------------------------\n";

            Bitboard b = gen.toBitboard();
            for (int y = 1; y <= 25; ++y) {
                for (int x = 1; x <= 25; ++x) {
                    f << (b.data[y] & (1 << (x - 1)) ? "C " : ". ");
                }
                f << "\n";
            }
            f.close();
        }

    public:
        void submit(const Genome& gen, const SimulationResult& res) {
            // X: Density (0.0 - 1.0) -> 0..19
            int ix = std::clamp((int)(res.pheno_x * 20), 0, 19);
            // Y: Distance (0.0 - 18.0) -> 0..19
            int iy = std::clamp((int)((res.pheno_y / 18.0) * 20), 0, 19);
            // It's better to select the multiplier manually.

            std::lock_guard<std::mutex> lock(archive_mtx);
            ArchiveCell& cell = grid[ix][iy];

            bool is_global_record = false;
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
            else { // Mana -> Blocks count
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

            for (int i = 0; i < 6; i++) {
                out_gen.mutationWeights[i] = (out_gen.mutationWeights[i] + cell.genome.mutationWeights[i]) / 2.0;
            }

            for (int i = 0; i < 5; i++) {
                out_gen.organs[i] = cell.genome.organs[i];
            }
            out_gen.organCount = cell.genome.organCount;

            cell.usage_count++;
            return true;
        }
    };
}