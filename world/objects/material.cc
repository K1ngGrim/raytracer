#include "material.h"

Material::Material(color materialColor, int type) {
    this->materialColor = materialColor;
    this->type = type;
    if(type < 0 || type > EMISSIVE)
        this->type = 0;
}

Material::Material(color materialColor, int type, float value) : Material(materialColor, type) {
    switch(this->type) {
        case GLOSSY:
            this->roughness = value < 0.f ? 0.f : (value > 1.f ? 1.f : value);
            break;
        case GLASS:
            this->refractionIndex = value;
            break;
        case EMISSIVE:
            this->emission = value;
            break;
        default:
            break;
    }
}

Material::Material(){}
