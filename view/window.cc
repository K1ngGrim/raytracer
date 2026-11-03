#include "window.h"
#include "../utility/bmp.h"
#include "../math/math.h"
#include <chrono>
#include <cstdio>

int Window::Run(const char *output_path) {
    auto start = std::chrono::steady_clock::now();
    Render();
    double render_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();

    // Primaerstrahlen gehen alle in die Suche nach dem naechsten Objekt, der Rest davon sind Folgestrahlen
    uint64_t primary = uint64_t(this->width) * uint64_t(this->height) * uint64_t(this->samples_per_pixel);
    printf("TIME: render %.1f ms\n", render_ms);
    printf("STATS: primary %llu, secondary %llu, shadow %llu, tests %llu\n",
           (unsigned long long) primary, (unsigned long long) (world->nearest_queries - primary),
           (unsigned long long) world->any_queries, (unsigned long long) world->intersection_tests);

    if (!save_bmp(output_path, int(this->width), int(this->height), this->pixels)) {
        printf("Error writing %s\n", output_path);
        return -1;
    }
    printf("DEBUG: Image saved as %s\n", output_path);
    return 1;
}

void Window::Render() {
    const int samples = this->samples_per_pixel < 1 ? 1 : this->samples_per_pixel;
    this->pixels.assign(size_t(int(this->width)) * size_t(int(this->height)), 0u);

    // Bildebene neu berechnen, damit Aenderungen an der Kamera (Position, Blickrichtung, Oeffnungswinkel) beruecksichtigt werden
    delete this->viewport;
    this->viewport = new Viewport(*cam, this->width, this->height);

    for (int j = 0; j < int(this->height); ++j) {
        for (int i = 0; i < int(this->width); ++i) {
            color p_color = {0.f, 0.f, 0.f};

            for (int s = 0; s < samples; ++s) {
                // bei mehreren Sehstrahlen zufaellig im Pixel, mit einem Strahl durch die Pixelmitte
                float di = samples > 1 ? random_float() - 0.5f : 0.f;
                float dj = samples > 1 ? random_float() - 0.5f : 0.f;

                Vector3df pixel_center = this->viewport->pixel00_loc + ((float(i) + di) * this->viewport->pixel_delta_u) + ((float(j) + dj) * this->viewport->pixel_delta_v);
                Vector3df ray_direction = pixel_center - cam->camera_center;
                Ray3df ray = {cam->camera_center, ray_direction};

                p_color = p_color + cam->ray_color(ray, world, this->max_depth);
            }
            p_color = p_color / float(samples);

            render_pixel(this->pixels, int(this->width), p_color, i, j);
        }

        if ((j + 1) % 32 == 0 || j + 1 == int(this->height)) {
            printf("\rRendering: %3d%%", (j + 1) * 100 / int(this->height));
            fflush(stdout);
        }
    }
    printf("\n");
}

Window::~Window() = default;
