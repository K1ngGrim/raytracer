#ifndef SHADER_H
#define SHADER_H

#include "../math/color.h"
#include "../scene/scene.h"

// Beleuchtungsmodell nach Whitted: direktes Licht nach Lambert und Phong mit dem Gewicht d (Deckkraft),
// dazu rekursiv der gespiegelte Strahl mit dem Gewicht Ks und der gebrochene Strahl mit dem Gewicht 1 - d.
class Shader {
public:
    explicit Shader(Scene &scene) : scene(scene) {}

    // Farbe, die entlang des Strahls ankommt. depth begrenzt die Rekursion, bei 0 ist das Ergebnis schwarz.
    color trace(const Ray3df &ray, int depth);

    // Ka + Summe ueber alle sichtbaren Lichtquellen von Kd cos(theta) + Ks cos(alpha)^Ns.
    // normal ist normiert und zeigt gegen den Strahl, to_viewer ist normiert.
    color direct_light(const Material &material, Vector3df point, Vector3df normal, Vector3df to_viewer);

    // Himmel als Verlauf von Weiss nach Hellblau
    static color background(const Ray3df &ray);

private:
    Scene &scene;
};

#endif
