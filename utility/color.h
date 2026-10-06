#ifndef COLOR_H
#define COLOR_H

#include "./../math/math.h"
#include <cstdint>
#include <vector>

//Saving colors from 0F..1F
using color = Vector3df;

// Schreibt die Farbe (0..1 pro Kanal) als 0x00RRGGBB an Position x,y in den Pixelpuffer (width Pixel pro Zeile)
void render_pixel(std::vector<uint32_t> &pixels, int width, color pixel_color, int x, int y);
Vector3df multiply(Vector3df first, Vector3df second);

#endif
