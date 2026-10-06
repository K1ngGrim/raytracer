#ifndef COLOR_H
#define COLOR_H

#include "./../math/math.h"
#include <SDL2/SDL.h>

//Saving colors from 0F..1F
using color = Vector3df;

void render_pixel(SDL_Renderer *renderer, color pixel_color, int x, int y);
Vector3df multiply(Vector3df first, Vector3df second);

#endif