#pragma once
#include <cstdint>
#include <algorithm>


namespace Dandelifeon {
    struct RelativePoint { int8_t dx, dy; };

    struct Structure {
        int8_t x, y;
        RelativePoint cells[10];
        int8_t count = 0;
        bool isObstacle = false;

        Structure() : x(12), y(12), count(0) {}

        void addPoint(int8_t dx, int8_t dy) {
            if (count < 10) 
                cells[count++] = { dx, dy };
        }

        void mirrorLocal(bool axisX, bool axisY) {
            for (int i = 0; i < count; ++i) {
                if (axisX) 
                    cells[i].dx = -cells[i].dx;

                if (axisY) 
                    cells[i].dy = -cells[i].dy;
            }
        }

        void rotate90() {
            for (int i = 0; i < count; ++i) {
                int8_t tmp = cells[i].dx;
                cells[i].dx = -cells[i].dy;
                cells[i].dy = tmp;
            }
        }
    };

}
