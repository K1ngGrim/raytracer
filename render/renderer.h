#ifndef RENDERER_H
#define RENDERER_H

#include "camera.h"
#include "shader.h"
#include <cstdint>
#include <vector>

// Schleife ueber alle Pixel: erzeugt mit der Kamera die Sehstrahlen und mittelt ihre Farben aus dem Shader
class Renderer {
public:
    int samples_per_pixel = 1;   // Sehstrahlen je Pixel, bei mehr als einem zufaellig im Pixel verteilt
    int max_depth = 5;           // hoechste Rekursionstiefe fuer Spiegelung und Brechung

    Renderer(Scene &scene, const Camera &camera) : camera(camera), shader(scene) {}

    // berechnet das Bild, je Pixel ein Wert 0x00RRGGBB, Zeile fuer Zeile von oben links
    std::vector<uint32_t> render();

private:
    const Camera &camera;
    Shader shader;
};

#endif
