#include "color.h"

void render_pixel(SDL_Renderer *renderer, color pixel_color, int x, int y) {
    SDL_SetRenderDrawColor(renderer, pixel_color[0]*255.f, pixel_color[1]*255.f, pixel_color[2]*255.f, 255);
    SDL_RenderDrawPoint(renderer, x, y);
}

Vector3df multiply(Vector3df first, Vector3df second) {
    Vector3df result = first;
    for(size_t t = 0u; t < 3u; t++) {
        result[t] *= second[t];
    }
    return result;
}
