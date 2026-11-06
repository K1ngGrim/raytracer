#ifndef LIGHT_H
#define LIGHT_H

#include "../math/math.h"
#include "../math/color.h"

// punktfoermige Lichtquelle
class PointLight {
public:
    Vector3df position = {0.f, 0.f, 0.f};
    color intensity = {1.f, 1.f, 1.f};

    explicit PointLight(Vector3df position) : position(position) {}
};

#endif
