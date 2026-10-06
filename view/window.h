#ifndef WINDOW_H
#define WINDOW_H

#include <SDL2/SDL.h>
#include "camera.h"
#include "viewport.h"

class Window
{
private:
    float aspect_ratio = 16.0f / 9.f;
public:
    SDL_Window *win{};
    SDL_Renderer *renderer{};
    bool running = true;
    bool changed = true;
    SDL_Event event{};
    Camera* cam;
    Viewport* viewport;
    World* world;
    float width, height;
    const char *windowTitle;

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
    
    bool Init();
    int Run();
    void Render() const;
    void PollEvents();

    // Destruktor, Deklaration
    ~Window();
};

#endif
