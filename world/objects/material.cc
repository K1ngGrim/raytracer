#include "material.h"

Material::Material(color materialColor, int type, float refractionIndex) {
    this->materialColor = materialColor;
    this->type = type;
    this->refractionIndex = refractionIndex;
    if(type < LAMBERTIAN || type > GLASS)
        this->type = LAMBERTIAN;
}

Material::Material(){}
