#pragma once
#include <cstring>

namespace Dandelifeon {
    const int S = 25;
    const int PS = 27;
    const int MAX_TICKS = 60;
    const long MANA_CAP = 50000;

    struct Point { int x, y; };

    struct Board {
        unsigned char g[PS][PS];
        void clear() { std::memset(g, 0, sizeof(g)); }
    };

    inline int step(Board& curr, Board& next, bool& wipe) {
        int inC = 0; wipe = false;
        for (int y = 1; y <= 25; ++y) {
            unsigned char* rC = curr.g[y], * rU = curr.g[y - 1], * rD = curr.g[y + 1], * rN = next.g[y];
            for (int x = 1; x <= 25; ++x) {
                int neighbors = rU[x - 1] + rU[x] + rU[x + 1] + rC[x - 1] + rC[x + 1] + rD[x - 1] + rD[x] + rD[x + 1];
                unsigned char live = (neighbors == 3) | (rC[x] & (neighbors == 2));
                rN[x] = live;
                if (live && x >= 12 && x <= 14 && y >= 12 && y <= 14) { wipe = true; inC++; }
            }
        }
        return inC;
    }
}