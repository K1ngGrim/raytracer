#include "material.h"

Material::Material(color materialColor, int type) {
    this->materialColor = materialColor;
    this->type = type;
    if(type < 0 || type > 1)
        this->type = 0;
}

Material::Material(){}
