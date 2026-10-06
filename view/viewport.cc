#include "viewport.h"
#include "camera.h"

Viewport::Viewport(Camera& cam, float w_width, float w_height) {
    this->vector_u = Vector3df{cam.viewport_width, 0, 0.f};
    this->vector_v = Vector3df{0, -cam.viewport_height, 0.f};

    this->pixel_delta_u = vector_u / w_width;
    this->pixel_delta_v = vector_v / w_height;

    this->viewport_upper_left = cam.camera_center - Vector3df{0.f, 0.f, cam.focal_length} - vector_u/2.f - vector_v/2.f;
    this->pixel00_loc = viewport_upper_left + (0.5f * (pixel_delta_u + pixel_delta_v));
}