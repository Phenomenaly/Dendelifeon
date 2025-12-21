#pragma once
#include "Dandelifeon.hpp"

enum SymType { ASYM, HR180, HMX, DIAG, QR90, QMXY };

class Symmetry {
public:
    static int apply(Dandelifeon::Board& b, int type, const Dandelifeon::Point* pts, int count) {
        int total = 0;
        auto set = [&](int px, int py) {
            if (px >= 0 && px < 25 && py >= 0 && py < 25 && !b.g[py + 1][px + 1]) {
                b.g[py + 1][px + 1] = 1; total++;
            }
            };

        for (int i = 0; i < count; ++i) {
            int x = pts[i].x, y = pts[i].y;
            switch (type) {
            case ASYM:  set(x, y); break;
            case HR180: set(x, y); set(24 - x, 24 - y); break;
            case HMX:   set(x, y); set(24 - x, y); break;
            case DIAG:  set(x, y); set(y, x); set(24 - x, 24 - y); set(24 - y, 24 - x); break;
            case QR90:  set(x, y); set(24 - y, x); set(24 - x, 24 - y); set(y, 24 - x); break;
            case QMXY:  set(x, y); set(24 - x, y); set(x, 24 - y); set(24 - x, 24 - y); break;
            }
        }
        return total;
    }

    static const char* getName(int type) {
        switch (type) {
        case ASYM: return "ASYM";
        case HR180: return "HR180";
        case HMX: return "HMX";
        case DIAG: return "DIAG";
        case QR90: return "QR90";
        case QMXY: return "QMXY";
        default: return "UNKNOWN";
        }
    }
};