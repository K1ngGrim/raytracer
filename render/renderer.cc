#include "renderer.h"
#include "../math/math.h"
#include <cstdio>

std::vector<uint32_t> Renderer::render() {
    const int width = this->camera.width(), height = this->camera.height();
    const int samples = this->samples_per_pixel < 1 ? 1 : this->samples_per_pixel;
    std::vector<uint32_t> pixels(size_t(width) * size_t(height), 0u);

    for(int y = 0; y < height; ++y) {
        for(int x = 0; x < width; ++x) {
            color pixel_color = {0.f, 0.f, 0.f};

            for(int s = 0; s < samples; ++s) {
                // bei mehreren Sehstrahlen zufaellig im Pixel, mit einem Strahl durch die Pixelmitte
                float dx = samples > 1 ? random_float() - 0.5f : 0.f;
                float dy = samples > 1 ? random_float() - 0.5f : 0.f;
                Ray3df ray = this->camera.primary_ray(float(x) + dx, float(y) + dy);
                pixel_color = pixel_color + this->shader.trace(ray, this->max_depth);
            }
            render_pixel(pixels, width, pixel_color / float(samples), x, y);
        }

        if((y + 1) % 32 == 0 || y + 1 == height) {
            printf("\rRendering: %3d%%", (y + 1) * 100 / height);
            fflush(stdout);
        }
    }
    printf("\n");
    return pixels;
}
