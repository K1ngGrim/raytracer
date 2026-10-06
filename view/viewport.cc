#include "viewport.h"
#include "camera.h"

Viewport::Viewport(Camera& cam, float w_width, float w_height) {
    // Basis der Kamera: w zeigt entgegen der Blickrichtung, u nach rechts, v nach oben
    // (fuer die Standardkamera mit Blick in -z ist das u = (1,0,0) und v = (0,1,0))
    Vector3df w = -1.f * cam.view_direction;
    w.normalize();
    Vector3df u = cam.up.cross_product(w);
    u.normalize();
    Vector3df v = w.cross_product(u);

    this->vector_u = cam.viewport_width * u;
    this->vector_v = -cam.viewport_height * v;

    this->pixel_delta_u = vector_u / w_width;
    this->pixel_delta_v = vector_v / w_height;

    this->viewport_upper_left = cam.camera_center - cam.focal_length * w - vector_u/2.f - vector_v/2.f;
    this->pixel00_loc = viewport_upper_left + (0.5f * (pixel_delta_u + pixel_delta_v));
}