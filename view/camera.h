#ifndef CAMERA_H
#define CAMERA_H

#include "../math/math.h"
#include "../utility/color.h"
#include "../world/world.h"
#include "../geometry/geometry.h"

class Camera {
public:
    float focal_length = 5.f;
    float viewport_height = 9.f;
    float viewport_width;
    Vector3df camera_center = {0.f, 0.f, 0.f};

    Camera(float height, float width) {
        this->viewport_width = this->viewport_height * double(width/height);
    }

    color ray_color(Ray3df ray, World* world, int depth);
    //color cast_ray(Ray3df ray, World* world, int depth);

    static float lambertian(World* world, Light light, Intersection_Context<float, 3> context);
};




#endif