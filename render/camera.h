#ifndef CAMERA_H
#define CAMERA_H

#include "../geometry/geometry.h"
#include "../math/math.h"

// Lochkamera mit Bildebene. Erzeugt fuer eine Stelle im Bild den Sehstrahl.
class Camera {
public:
    // Bild mit width x height Pixeln, Kamera im Ursprung mit Blick in -z
    Camera(int width, int height);

    // stellt die Kamera an position mit Blick auf target, fov_degrees ist der vertikale Oeffnungswinkel
    void place(Vector3df position, Vector3df target, float fov_degrees);

    // Sehstrahl durch die Bildstelle (x, y) in Pixeln, (0, 0) ist die Mitte des Pixels oben links.
    // Die Richtung ist nicht normiert.
    Ray3df primary_ray(float x, float y) const;

    int width() const { return this->image_width; }
    int height() const { return this->image_height; }
    Vector3df position() const { return this->origin; }

private:
    int image_width, image_height;
    Vector3df origin = {0.f, 0.f, 0.f};
    Vector3df pixel00 = {0.f, 0.f, 0.f};         // Mitte des Pixels oben links auf der Bildebene
    Vector3df pixel_delta_u = {0.f, 0.f, 0.f};   // ein Pixel nach rechts
    Vector3df pixel_delta_v = {0.f, 0.f, 0.f};   // ein Pixel nach unten
};

#endif
