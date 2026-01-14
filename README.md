# Dandelifeon Solver

A C++ solver designed to find optimal Game of Life patterns for the Dandelifeon flower (from the Botania mod for the Minecraft). The project utilizes **AVX2 intrinsics** and a **MAP-Elites** evolutionary algorithm to discover configurations that maximize mana generation efficiency within a constrained 25x25 grid.

### 1. Simulation Engine (`DandelifeonEngine`)
The core simulation is built on bitwise parallelism using **AVX2 (256-bit SIMD)** instructions.
*   **Data Structure:** The 25x25 grid is mapped to a `uint32_t` array, allowing simultaneous processing of row neighbors.
*   The engine processes two distinct bitboards:
    *   **Life Layer** - is standard Conway's Game of Life automaton.
    *   **Obstacle Layer** - is static bitmask that eliminates any overlapping life cells each tick.

### 2. Evolutionary Algorithm (MAP-Elites)
Instead of a single objective optimization, the solver uses a Quality Diversity (QD) algorithm known as **MAP-Elites**. 
In **Archive** I store 20x20 best-performing genomes found so far.

* **Phenotype X:** Pattern Density (Compactness). It was necessary for the asymmetric pattern to determine whether it is better to use distant or close structures.

* **Phenotype Y:** Average Distance from Center. Sometimes successful patterns circle around the center and only reach it at the end.

In fact, on average, the winner comes from any square on this map, meaning the values ​​are incorrect. The measurements I chose were based on the fact that I can't take values ​​related to the resulting mana or the number of squares involved, since I'm looking for the maximum/minimum in these measurements a priori.


### 3. Mutation Strategy
The `EvolutionManager` applies mutations based on adaptive weights.
*   **Positional mutations** Shift board, shift structure, shift individual cell.
*   **Topology mutations** Mirror structure, rotate structure, toggle global symmetry.
*   **Composition mutations** add/remove life cells or add/reemove walls.
*   **"Smart Wall" mutation** The engine tracks the "footprint" of life over the entire simulation. A specialized mutation places obstacles specifically on coordinates with high historical activity to redirect flow.
---

## Dandelifeon Constraints
The solver adheres to specific Botania mechanics:
*   **Grid:** 25x25 cells.
*   **Absorption:** Cells entering the center 3x3 area are consumed.
*   **Scoring:** $Mana = Cells \times Age \times 150$.
*   **Aging:** New cells inherit the age of their oldest neighbor + 1.

---

## Configuration & Usage

Configure the search parameters in `main.cpp` via `startCustomOptimization`:

| Parameter | Description |
| :--- | :--- |
| `maxTicks` | Max generations before pattern expiration (Default: 60) |
| `manaPerTick` | Base mana multiplier (Default: 150) |
| `manaCap` | Dandelifeon internal buffer limit (Default: 50000) |
| `threads` | Automatically scales to utilize available CPU cores |

In the current commit I was looking for a solution for the changed rules of new versions (1.20+)
