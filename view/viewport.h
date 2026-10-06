#ifndef VIEWPORT_H
#define VIEWPORT_H

#include "../geometry/geometry.h"
#include "camera.h"

class Viewport {
public:
    Vector3df vector_u = {0.f, 0.f, 0.f};
    Vector3df vector_v = {0.f, 0.f, 0.f};

    Vector3df pixel_delta_u = {0.f, 0.f, 0.f};
    Vector3df pixel_delta_v = {0.f, 0.f, 0.f};

    Vector3df viewport_upper_left = {0.f, 0.f, 0.f};
    Vector3df pixel00_loc = {0.f, 0.f, 0.f};

    Viewport(Camera& cam, float w_width, float w_height);

};


#endif
