#ifndef MATERIAL_H
#define MATERIAL_H

#include "../../utility/color.h"

// Materialtypen
const int LAMBERTIAN = 0;   // diffus, Lambert-Shading mit Schatten
const int REFLECTIVE = 1;   // Spiegel, die Reflexion wird mit materialColor eingefaerbt
const int GLASS = 2;        // durchsichtig, Brechung und Reflexion nach Fresnel (Schlick-Naeherung)

class Material {
public:
    color materialColor = {1.f, 1.f, 1.f};
    int type = -1;
    float refractionIndex = 1.5f;   // nur fuer GLASS

    Material();

    Material(color materialColor, int type, float refractionIndex = 1.5f);

};


#endif
