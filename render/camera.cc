#include "camera.h"
#include <cmath>

Camera::Camera(int width, int height) : image_width(width), image_height(height) {
    // Bildebene der Hoehe 9 im Abstand 5, wie in der Szene aus Kugeln vorgesehen
    this->place({0.f, 0.f, 0.f}, {0.f, 0.f, -1.f}, 2.f * std::atan(0.9f) * 180.f / float(M_PI));
}

void Camera::place(Vector3df position, Vector3df target, float fov_degrees) {
    const float focal_length = 5.f;
    float viewport_height = 2.f * focal_length * std::tan(fov_degrees * float(M_PI) / 360.f);
    float viewport_width = viewport_height * float(this->image_width) / float(this->image_height);
    this->origin = position;

    // Basis der Kamera: w zeigt entgegen der Blickrichtung, u nach rechts, v nach oben
    Vector3df w = position - target;
    w.normalize();
    Vector3df up = {0.f, 1.f, 0.f};
    Vector3df u = up.cross_product(w);
    u.normalize();
    Vector3df v = w.cross_product(u);

    Vector3df across = viewport_width * u;
    Vector3df down = -viewport_height * v;
    this->pixel_delta_u = across / float(this->image_width);
    this->pixel_delta_v = down / float(this->image_height);

    Vector3df upper_left = position - focal_length * w - across / 2.f - down / 2.f;
    this->pixel00 = upper_left + 0.5f * (this->pixel_delta_u + this->pixel_delta_v);
}

Ray3df Camera::primary_ray(float x, float y) const {
    Vector3df pixel = this->pixel00 + x * this->pixel_delta_u + y * this->pixel_delta_v;
    return {this->origin, pixel - this->origin};
}
