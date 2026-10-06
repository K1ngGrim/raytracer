#ifndef WINDOW_H
#define WINDOW_H

#include <cstdint>
#include <vector>
#include "camera.h"
#include "viewport.h"

// Der "Bildschirm": rendert die Szene in einen Pixelpuffer und schreibt ihn als BMP-Datei.
class Window
{
private:
    float aspect_ratio = 16.0f / 9.f;
public:
    std::vector<uint32_t> pixels; // 0x00RRGGBB, Zeile fuer Zeile von oben links
    Camera* cam;
    Viewport* viewport;
    World* world;
    float width, height;
    const char *windowTitle;
    int samples_per_pixel = 1;   // Strahlen pro Pixel (zufaellig im Pixel verteilt, Mittelwert), bei Path-Tracing viele (z.B. 64+)
    int max_depth = 10;          // maximale Rekursionstiefe (Reflexionen, Brechungen, Bounces)
    unsigned threads = 0;        // Anzahl der Threads, 0: alle Kerne
    float exposure = 1.f;        // Helligkeitsfaktor, mit dem jede Pixelfarbe multipliziert wird

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

    // wie oben, aber mit frei waehlbarer Hoehe (das Seitenverhaeltnis ergibt sich aus Breite und Hoehe)
    Window(const char *title, float w, float h, World* world) : Window(title, w, world)
    {
        this->height = h < 1 ? 1 : h;
        this->aspect_ratio = this->width / this->height;
        this->cam = new Camera(this->height, this->width);
        this->viewport = new Viewport(*cam, this->width, this->height);
    }

    // Rendert das Bild und speichert es als BMP. Gibt 1 bei Erfolg und -1 bei einem Schreibfehler zurueck.
    int Run(const char *output_path = "raytracer.bmp");
    void Render();

    // Destruktor, Deklaration
    ~Window();
};

#endif
