#ifndef MATERIAL_H
#define MATERIAL_H

#include "../../utility/color.h"

//int type: 0 (Lambertian) 1 (Reflective)

class Material {
public:
    color materialColor = {1.f, 1.f, 1.f};
    int type = -1;

    Material();

    Material(color materialColor, int type);

};


#endif
