#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>
#include <vector>
#include "camera.h"
#include "viewport.h"

class Window
{
private:
    float aspect_ratio = 16.0f / 9.f;
public:
    std::vector<uint32_t> pixels; // 0x00RRGGBB, Zeile fuer Zeile von oben links
    bool changed = true;
    Camera* cam;
    Viewport* viewport;
    World* world;
    float width, height;
    const char *windowTitle;
    int samples_per_pixel = 1;   // Sehstrahlen pro Pixel, zufaellig im Pixel verteilt, die Farben werden gemittelt
    int max_depth = 5;           // maximale Rekursionstiefe fuer Reflexion und Brechung

    // Konstruktor, Deklaration
    Window(const char *title, float w, World* world)
    {
        this->windowTitle = title;
        this->width = w;
        this->world = world;
        this->height = float(w / this->aspect_ratio) < 1 ? 1 : float(w / this->aspect_ratio);
        this->cam = new Camera(this->height, this->width);
        this->viewport = new Viewport(*cam, this->width, this->height);
    }

    // wie oben, aber mit frei waehlbarer Hoehe (z.B. 1024 x 768)
    Window(const char *title, float w, float h, World* world) : Window(title, w, world)
    {
        this->height = h < 1 ? 1 : h;
        this->aspect_ratio = this->width / this->height;
        this->cam = new Camera(this->height, this->width);
        this->viewport = new Viewport(*cam, this->width, this->height);
    }

    // Rendert das Bild und schreibt es als BMP-Datei. Gibt 1 bei Erfolg und -1 bei einem Fehler zurueck.
    int Run(const char *output_path = "raytracer.bmp");
    void Render();

    // Destruktor, Deklaration
    ~Window();
};

#endif
