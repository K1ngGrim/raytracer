#include "material.h"

Material Material::matte(color diffuse) {
    Material material;
    material.ambient = 0.1f * diffuse;
    material.diffuse = 0.9f * diffuse;
    return material;
}

Material Material::mirror(color tint) {
    Material material;
    material.ambient = {0.f, 0.f, 0.f};
    material.diffuse = {0.f, 0.f, 0.f};
    material.specular = tint;
    material.shininess = 200.f;
    return material;
}

bool Material::reflects() const {
    return this->specular[0] > 0.f || this->specular[1] > 0.f || this->specular[2] > 0.f;
}

bool Material::transmits() const {
    return this->transparency > 0.f;
}
