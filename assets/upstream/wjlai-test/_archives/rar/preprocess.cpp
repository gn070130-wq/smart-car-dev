#include "preprocess.h"

#define SRC_WIDTH   160
#define SRC_HEIGHT  120
#define OUT_WIDTH   140
#define OUT_HEIGHT  100
#define CROP_TOP    20
#define CROP_LEFT   10

void crop_and_resize_bilinear(const uint8_t* src, uint8_t* dst) {
    for (int y = 0; y < OUT_HEIGHT; ++y) {
        int src_y = y + CROP_TOP;
        const uint8_t* src_row = src + src_y * SRC_WIDTH;
        uint8_t* dst_row = dst + y * OUT_WIDTH;
        for (int x = 0; x < OUT_WIDTH; ++x) {
            int src_x = x + CROP_LEFT;
            dst_row[x] = src_row[src_x];
        }
    }
}

Preprocess preprocess;


float calculate_slopeK(int y1, int y2, int x1, int x2)
{
    if(x1 - x2 == 0) {
        return 0.0f;
    }
    return (y1 - y2) * 1.0 / (x1 - x2) * 1.0;
}

float calculate_slopeB(float k, int y, int x)
{
    return (y * 1.0 - k * x);
}

