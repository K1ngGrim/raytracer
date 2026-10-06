#include "window.h"
#include "../utility/bmp.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

int Window::Run(const char *output_path) {
    auto start = std::chrono::steady_clock::now();
    Render();
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    printf("DEBUG: Rendered %dx%d with %d sample(s) per pixel in %.1f s\n", int(this->width), int(this->height), this->samples_per_pixel, seconds);

    if (!save_bmp(output_path, int(this->width), int(this->height), this->pixels)) {
        printf("Error writing %s\n", output_path);
        return -1;
    }
    printf("DEBUG: Image saved as %s\n", output_path);
    return 1;
}

void Window::Render() {
    const int w = int(this->width);
    const int h = int(this->height);
    const int samples = std::max(1, this->samples_per_pixel);
    this->pixels.assign(size_t(w) * size_t(h), 0u);

    // Bildebene neu berechnen, damit Aenderungen an der Kamera (Position, Blickrichtung, Oeffnungswinkel) beruecksichtigt werden
    delete this->viewport;
    this->viewport = new Viewport(*cam, this->width, this->height);

    if (this->use_gpu) {
        if (this->RenderGpu())
            return;
        printf("DEBUG: GPU rendering failed, falling back to the CPU\n");
    }

    // Die Pixel sind voneinander unabhaengig und die Welt wird nur gelesen: Zeilen werden auf alle Kerne verteilt.
    std::atomic<int> next_row{0};
    std::atomic<int> rows_done{0};
    auto render_rows = [&]() {
        for (int j = next_row++; j < h; j = next_row++) {
            for (int i = 0; i < w; ++i) {
                seed_random(uint32_t(j) * uint32_t(w) + uint32_t(i));   // pro Pixel: gleiches Bild bei jedem Lauf

                color p_color = {0.f, 0.f, 0.f};
                for (int s = 0; s < samples; ++s) {
                    // bei mehreren Samples zufaellig im Pixel, mit einem Sample durch die Pixelmitte
                    float di = samples > 1 ? random_float() - 0.5f : 0.f;
                    float dj = samples > 1 ? random_float() - 0.5f : 0.f;
                    Vector3df pixel_center = this->viewport->pixel00_loc + ((float(i) + di) * this->viewport->pixel_delta_u) + ((float(j) + dj) * this->viewport->pixel_delta_v);
                    Vector3df ray_direction = pixel_center - cam->camera_center;
                    Ray3df ray = {cam->camera_center, ray_direction};

                    p_color = p_color + cam->ray_color(ray, world, this->max_depth);
                }
                p_color = (this->exposure / float(samples)) * p_color;

                if (cam->path_tracing) {   // Gamma-Korrektur (gamma 2)
                    for (size_t k = 0; k < 3; ++k)
                        p_color[k] = std::sqrt(std::max(0.f, p_color[k]));
                }
                render_pixel(this->pixels, w, p_color, i, j);
            }

            int done = ++rows_done;
            if (done % 16 == 0 || done == h) {
                printf("\rRendering: %3d%%", done * 100 / h);
                fflush(stdout);
            }
        }
    };

    unsigned thread_count = this->threads > 0 ? this->threads : std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    for (unsigned t = 0; t < thread_count; ++t)
        threads.emplace_back(render_rows);
    for (std::thread & thread : threads)
        thread.join();
    printf("\n");
}

#ifndef USE_METAL
bool Window::RenderGpu() {
    printf("Error: GPU rendering needs Metal and is only available on macOS\n");
    return false;
}
#endif

Window::~Window() = default;
