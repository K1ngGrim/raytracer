#ifndef CAMERA_H
#define CAMERA_H

#include <cmath>
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
    Vector3df view_direction = {0.f, 0.f, -1.f};   // Blickrichtung der Kamera
    Vector3df up = {0.f, 1.f, 0.f};
    float aspect_ratio;

    Camera(float height, float width) {
        this->viewport_width = this->viewport_height * double(width/height);
        this->aspect_ratio = width / height;
    }

    // dreht die Kamera so, dass sie auf target blickt
    void look_at(Vector3df target) {
        this->view_direction = target - this->camera_center;
        this->view_direction.normalize();
    }

    // setzt den vertikalen Oeffnungswinkel (in Grad) ueber die Hoehe der Bildebene
    void set_fov(float degrees) {
        this->viewport_height = 2.f * this->focal_length * std::tan(degrees * float(M_PI) / 360.f);
        this->viewport_width = this->viewport_height * this->aspect_ratio;
    }

    color ray_color(Ray3df ray, World* world, int depth);
    //color cast_ray(Ray3df ray, World* world, int depth);

    static float lambertian(World* world, Light light, Intersection_Context<float, 3> context);
};




#endif