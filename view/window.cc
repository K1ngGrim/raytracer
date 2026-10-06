#include "window.h"

void takeScreenshot(Window *win) {
    SDL_Surface *sshot = SDL_CreateRGBSurface(0, win->width, win->height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
    SDL_RenderReadPixels(win->renderer, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);

    time_t now = time(0);
    std::string dt = ctime(&now);
    std::replace(dt.begin(), dt.end(), ':', '-');
    std::replace(dt.begin(), dt.end(), ' ', '-');

    dt = "./screenshots/screenshot" + dt;
    dt = dt + ".bmp";
    SDL_SaveBMP(sshot, dt.c_str());
    SDL_FreeSurface(sshot);
}

bool Window::Init() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("Error initializing SDL2: %s\n", SDL_GetError());
		return false;
	}
	printf("DEBUG: SDL Successfully Initialized!\n");

	this->win = SDL_CreateWindow(
		this->windowTitle,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		this->width,
		this->height,
		0
	);

	if (!this->win) {
		printf("Error initializing Window: %s\n", SDL_GetError());
		return false;
	}
	printf("DEBUG: Window Successfully Initialized!\n");


	this->renderer = SDL_CreateRenderer(
		this->win,
		-1,
		SDL_RENDERER_ACCELERATED
	);


	if (!this->renderer) {
		printf("Error initializing Renderer: %s\n", SDL_GetError());
		return false;
	}
	printf("DEBUG: Renderer Successfully Initialized!\n");
	return true;
}

int Window::Run() {
	if (!this->Init()) {
		this->running = false;
		return -1;
	}

    uint a, b, delta;
    b = SDL_GetTicks();
	Render();
    while (this->running) {
        PollEvents();
    	Render();
    	takeScreenshot(this);
    	break;
    }
	return 1;
}



void Window::PollEvents() {
	while (SDL_PollEvent(&this->event)) {
		switch (event.type) {
		case SDL_QUIT:
			this->running = false;
			break;
        case SDL_KEYDOWN:
            switch( event.key.keysym.sym ){
                case SDLK_LEFT: {
                    Vector3df p = {0.5f, 0.f, 0.f};
                    Vector3df v = this->cam->camera_center + p;
                    this->cam->camera_center = v;
                    break;
                }
                case SDLK_RIGHT: {
                    Vector3df p = {-0.5f, 0.f, 0.f};
                    Vector3df v = this->cam->camera_center + p;
                    this->cam->camera_center = v;
                    break;
                }
                case SDLK_F10: {
                    takeScreenshot(this);
                }
            }
            break;
		}
	}
}

void Window::Render() const {

    for (int j = 0; j < int(this->height); ++j) {
        for (int i = 0; i < int(this->width); ++i) {
            Vector3df pixel_center = this->viewport->pixel00_loc + (float(i) * this->viewport->pixel_delta_u) + (float(j) * this->viewport->pixel_delta_v);
            Vector3df ray_direction = pixel_center - cam->camera_center;
            Ray3df ray = {cam->camera_center, ray_direction};

            color p_color = cam->ray_color(ray, world, 10);
            render_pixel(this->renderer, p_color, i, j);
        }
    }
    SDL_RenderPresent(this->renderer);
}

Window::~Window() = default;


