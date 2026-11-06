#ifndef MATERIAL_H
#define MATERIAL_H

#include "../math/color.h"

// Materialwerte wie in einer Wavefront-Materialdatei (.mtl). Alle Werte gehen in das Beleuchtungsmodell
// nach Whitted ein: Ka ueberall, Kd und das Glanzlicht nur bei sichtbarem Licht, Ks zusaetzlich als Gewicht
// des gespiegelten Strahls. d ist wie in der MTL-Spezifikation die Deckkraft der Oberflaeche: Ka, Kd und
// Glanzlicht zaehlen nur mit d, der gebrochene Strahl mit 1 - d.
class Material {
public:
    color ambient = {0.1f, 0.1f, 0.1f};    // Ka
    color diffuse = {0.8f, 0.8f, 0.8f};    // Kd
    color specular = {0.f, 0.f, 0.f};      // Ks, Glanzlicht und Spiegelung
    float shininess = 0.f;                 // Ns, Exponent des Glanzlichts
    float transparency = 0.f;              // 1 - d, Anteil des gebrochenen Strahls
    float refraction_index = 1.5f;         // Ni

    Material() = default;

    // diffuses Material, das auch im Schatten ein Zehntel seiner Farbe behaelt
    static Material matte(color diffuse);
    // Spiegel, der das gespiegelte Licht mit tint einfaerbt
    static Material mirror(color tint);

    bool reflects() const;   // Ks > 0
    bool transmits() const;  // d < 1
};

#endif
