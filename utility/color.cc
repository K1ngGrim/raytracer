#include "color.h"

void render_pixel(std::vector<uint32_t> &pixels, int width, color pixel_color, int x, int y) {
    // auf 0..1 begrenzen (NaN wird zu 0), dann wie bisher mit 255 multiplizieren und den Nachkommaanteil verwerfen
    auto channel = [](float c) -> uint32_t {
        c = (c > 0.f) ? (c < 1.f ? c : 1.f) : 0.f;
        return uint32_t(c * 255.f);
    };
    pixels[size_t(y) * size_t(width) + size_t(x)] =
        (channel(pixel_color[0]) << 16) | (channel(pixel_color[1]) << 8) | channel(pixel_color[2]);
}

Vector3df multiply(Vector3df first, Vector3df second) {
    Vector3df result = first;
    for(size_t t = 0u; t < 3u; t++) {
        result[t] *= second[t];
    }
    return result;
}
