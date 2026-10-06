#ifndef MATERIAL_H
#define MATERIAL_H

#include "../../utility/color.h"

//int type: 0 (Lambertian) 1 (Reflective) 2 (Glossy) 3 (Glass) 4 (Emissive)
enum MaterialType { LAMBERTIAN = 0, REFLECTIVE = 1, GLOSSY = 2, GLASS = 3, EMISSIVE = 4 };

class Material {
public:
    color materialColor = {1.f, 1.f, 1.f};
    int type = -1;

    // Zusatzparameter, je nach type:
    float roughness = 0.f;          // GLOSSY: 0 = perfekter Spiegel, 1 = sehr unscharfe Reflexion
    float refractionIndex = 1.5f;   // GLASS: Brechungsindex (Luft 1.0, Wasser 1.33, Glas 1.5, Diamant 2.4)
    float emission = 1.f;           // EMISSIVE: Leuchtstaerke (Faktor auf materialColor)

    Material();

    Material(color materialColor, int type);

    // value setzt den zum type passenden Zusatzparameter (roughness, refractionIndex oder emission)
    Material(color materialColor, int type, float value);

};


#endif
