#include "bmp.h"
#include <cstdio>

namespace {
void put_u16(std::vector<uint8_t> &out, uint16_t value) {
    out.push_back(uint8_t(value & 0xff));
    out.push_back(uint8_t(value >> 8));
}

void put_u32(std::vector<uint8_t> &out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(uint8_t((value >> shift) & 0xff));
    }
}
}

bool save_bmp(const char *filename, int width, int height, const std::vector<uint32_t> &pixels) {
    if (width <= 0 || height <= 0 || pixels.size() != size_t(width) * size_t(height)) {
        return false;
    }

    const uint32_t header_size = 14 + 40;                         // BITMAPFILEHEADER + BITMAPINFOHEADER
    const uint32_t row_size = (uint32_t(width) * 3u + 3u) & ~3u;  // jede Zeile ist auf ein Vielfaches von 4 Byte aufgefuellt
    const uint32_t data_size = row_size * uint32_t(height);

    std::vector<uint8_t> out;
    out.reserve(header_size + data_size);


    out.push_back('B');
    out.push_back('M');
    put_u32(out, header_size + data_size);
    put_u32(out, 0);
    put_u32(out, header_size);

    put_u32(out, 40);
    put_u32(out, uint32_t(width));
    put_u32(out, uint32_t(height));  // positive Hoehe: Zeilen werden von unten nach oben gespeichert
    put_u16(out, 1);                 // Farbebenen
    put_u16(out, 24);                // Bits pro Pixel
    put_u32(out, 0);                 // keine Kompression
    put_u32(out, data_size);
    put_u32(out, 2835);              // 72 dpi
    put_u32(out, 2835);
    put_u32(out, 0);
    put_u32(out, 0);

    for (int y = height - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const uint32_t pixel = pixels[size_t(y) * size_t(width) + size_t(x)];
            out.push_back(uint8_t(pixel & 0xff));          // Blau
            out.push_back(uint8_t((pixel >> 8) & 0xff));   // Gruen
            out.push_back(uint8_t((pixel >> 16) & 0xff));  // Rot
        }
        for (uint32_t padding = row_size - uint32_t(width) * 3u; padding > 0; --padding) {
            out.push_back(0);
        }
    }

    FILE *file = std::fopen(filename, "wb");
    if (!file) {
        return false;
    }
    bool ok = std::fwrite(out.data(), 1, out.size(), file) == out.size();
    if (std::fclose(file) != 0) {
        ok = false;
    }
    return ok;
}
