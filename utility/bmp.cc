#include "bmp.h"
#include <cstdio>

namespace {
// BMP speichert alle Zahlen als Little Endian
void put_u16(std::vector<uint8_t> &out, uint16_t value) {
    out.push_back(uint8_t(value & 0xff));
    out.push_back(uint8_t(value >> 8));
}

void put_u32(std::vector<uint8_t> &out, uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(uint8_t((value >> shift) & 0xff));
    }
}

const uint64_t HEADER_SIZE = 14 + 40;   // BITMAPFILEHEADER + BITMAPINFOHEADER

uint64_t row_size_of(int width) {
    return (uint64_t(width) * 3u + 3u) & ~uint64_t(3u);   // jede Zeile ist auf ein Vielfaches von 4 Byte aufgefuellt
}
}

bool bmp_fits(int width, int height) {
    if (width <= 0 || height <= 0)
        return false;
    return HEADER_SIZE + row_size_of(width) * uint64_t(height) <= UINT32_MAX;
}

bool save_bmp(const char *filename, int width, int height, const std::vector<uint32_t> &pixels) {
    if (!bmp_fits(width, height) || pixels.size() != size_t(width) * size_t(height)) {
        return false;
    }

    const uint64_t row_size = row_size_of(width);
    const uint32_t data_size = uint32_t(row_size * uint64_t(height));

    std::vector<uint8_t> header;
    header.reserve(HEADER_SIZE);

    // BITMAPFILEHEADER
    header.push_back('B');
    header.push_back('M');
    put_u32(header, uint32_t(HEADER_SIZE) + data_size);
    put_u32(header, 0);
    put_u32(header, uint32_t(HEADER_SIZE));

    // BITMAPINFOHEADER
    put_u32(header, 40);
    put_u32(header, uint32_t(width));
    put_u32(header, uint32_t(height));  // positive Hoehe: Zeilen werden von unten nach oben gespeichert
    put_u16(header, 1);                 // Farbebenen
    put_u16(header, 24);                // Bits pro Pixel
    put_u32(header, 0);                 // keine Kompression
    put_u32(header, data_size);
    put_u32(header, 2835);              // 72 dpi
    put_u32(header, 2835);
    put_u32(header, 0);
    put_u32(header, 0);

    FILE *file = std::fopen(filename, "wb");
    if (!file) {
        return false;
    }
    bool ok = std::fwrite(header.data(), 1, header.size(), file) == header.size();

    // zeilenweise schreiben, damit nicht die ganze Datei zusaetzlich im Speicher liegen muss
    std::vector<uint8_t> row(row_size, 0);   // die Fuellbytes am Zeilenende bleiben 0
    for (int y = height - 1; y >= 0 && ok; --y) {
        const uint32_t *source = pixels.data() + size_t(y) * size_t(width);
        for (int x = 0; x < width; ++x) {
            const uint32_t pixel = source[x];
            row[size_t(x) * 3 + 0] = uint8_t(pixel & 0xff);          // Blau
            row[size_t(x) * 3 + 1] = uint8_t((pixel >> 8) & 0xff);   // Gruen
            row[size_t(x) * 3 + 2] = uint8_t((pixel >> 16) & 0xff);  // Rot
        }
        ok = std::fwrite(row.data(), 1, row.size(), file) == row.size();
    }

    if (std::fclose(file) != 0) {
        ok = false;
    }
    return ok;
}
