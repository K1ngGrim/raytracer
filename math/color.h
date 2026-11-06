#ifndef COLOR_H
#define COLOR_H

#include "math.h"
#include <cstdint>
#include <vector>

//Saving colors from 0F..1F
using color = Vector3df;

// Schreibt die Farbe als 0x00RRGGBB an die Stelle x,y des Pixelpuffers (width Pixel pro Zeile)
void render_pixel(std::vector<uint32_t> &pixels, int width, color pixel_color, int x, int y);
Vector3df multiply(Vector3df first, Vector3df second);

#endif
