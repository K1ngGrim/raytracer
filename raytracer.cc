#include <cmath>
#include "geometry/geometry.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <ctime>
#include "view/window.h"
#include "view/camera.h"
#include "utility/color.h"
#include "utility/light.h"
#include "world/world.h"
#include "utility/obj_loader.h"
#include <cstdlib>
#include <string>

using namespace std;

// Die folgenden Kommentare beschreiben Datenstrukturen und Funktionen
// Die Datenstrukturen und Funktionen die weiter hinten im Text beschrieben sind,
// hängen höchstens von den vorhergehenden Datenstrukturen ab, aber nicht umgekehrt.



// Ein "Bildschirm", der das Setzen eines Pixels kapselt
// Der Bildschirm hat eine Auflösung (Breite x Höhe)
// Kann zur Ausgabe einer PPM-Datei verwendet werden oder
// mit SDL2 implementiert werden.



// Eine "Kamera", die von einem Augenpunkt aus in eine Richtung senkrecht auf ein Rechteck (das Bild) zeigt.
// Für das Rechteck muss die Auflösung oder alternativ die Pixelbreite und -höhe bekannt sein.
// Für ein Pixel mit Bildkoordinate kann ein Sehstrahl erzeugt werden.



// Für die "Farbe" benötigt man nicht unbedingt eine eigene Datenstruktur.
// Sie kann als Vector3df implementiert werden mit Farbanteil von 0 bis 1.
// Vor Setzen eines Pixels auf eine bestimmte Farbe (z.B. 8-Bit-Farbtiefe),
// kann der Farbanteil mit 255 multipliziert  und der Nachkommaanteil verworfen werden.


// Das "Material" der Objektoberfläche mit ambienten, diffusem und reflektiven Farbanteil.



// Ein "Objekt", z.B. eine Kugel oder ein Dreieck, und dem zugehörigen Material der Oberfläche.
// Im Prinzip ein Wrapper-Objekt, das mindestens Material und geometrisches Objekt zusammenfasst.
// Kugel und Dreieck finden Sie in geometry.h/tcc


// verschiedene Materialdefinition, z.B. Mattes Schwarz, Mattes Rot, Reflektierendes Weiss, ...
// im wesentlichen Variablen, die mit Konstruktoraufrufen initialisiert werden.


// Die folgenden Werte zur konkreten Objekten, Lichtquellen und Funktionen, wie Lambertian-Shading
// oder die Suche nach einem Sehstrahl für das dem Augenpunkt am nächsten liegenden Objekte,
// können auch zusammen in eine Datenstruktur für die gesammte zu
// rendernde "Szene" zusammengefasst werden.

// Die Cornelbox aufgebaut aus den Objekten
// Am besten verwendet man hier einen std::vector< ... > von Objekten.

// Punktförmige "Lichtquellen" können einfach als Vector3df implementiert werden mit weisser Farbe,
// bei farbigen Lichtquellen müssen die entsprechenden Daten in Objekt zusammengefaßt werden
// Bei mehreren Lichtquellen können diese in einen std::vector gespeichert werden.

// Sie benötigen eine Implementierung von Lambertian-Shading, z.B. als Funktion
// Benötigte Werte können als Parameter übergeben werden, oder wenn diese Funktion eine Objektmethode eines
// Szene-Objekts ist, dann kann auf die Werte teilweise direkt zugegriffen werden.
// Bei mehreren Lichtquellen muss der resultierende diffuse Farbanteil durch die Anzahl Lichtquellen geteilt werden.

// Für einen Sehstrahl aus allen Objekte, dasjenige finden, das dem Augenpunkt am nächsten liegt.
// Am besten einen Zeiger auf das Objekt zurückgeben. Wenn dieser nullptr ist, dann gibt es kein sichtbares Objekt.

// Die rekursive raytracing-Methode. Am besten ab einer bestimmten Rekursionstiefe (z.B. als Parameter übergeben) abbrechen.

Camera* cam;

// Kommandozeile (alle Parameter sind optional, die Szene selbst ist nicht einstellbar):
//   raytracer [scene.obj] [Optionen]
//   ohne scene.obj wird die eingebaute Szene aus Kugeln gerendert
static void print_usage() {
    printf(
        "Usage: raytracer [scene.obj] [options]\n"
        "  scene.obj          Wavefront-Szene (sonst die eingebaute Szene aus Kugeln)\n"
        "\n"
        "Bild\n"
        "  -w <pixel>         Breite  (Standard 1280)\n"
        "  -h <pixel>         Hoehe   (Standard: Breite * 9/16; nur -h: Breite = Hoehe * 16/9)\n"
        "  -s <n>             Samples pro Pixel (Standard 1, mit --pt 64)\n"
        "  -d <n>             maximale Rekursionstiefe fuer Reflexion, Brechung und Bounces (Standard 10)\n"
        "  -o <datei.bmp>     Ausgabedatei (Standard raytracer.bmp)\n"
        "  -t <n>             Anzahl Threads (Standard: alle Kerne)\n"
        "\n"
        "Verfahren\n"
        "  --pt               Path-Tracing statt Whitted: diffuse Bounces, unscharfe Reflexion bei glossy, Gamma-Korrektur\n"
        "  --badouel          Dreiecke mit Badouel statt Moeller-Trumbore schneiden (nur CPU, die GPU nutzt Moeller-Trumbore)\n"
        "  --gpu              auf der GPU rendern (Metal, nur macOS), -t wird dann ignoriert\n"
        "\n"
        "Kamera und Licht\n"
        "  --cam x y z        Kameraposition\n"
        "  --look x y z       Punkt, auf den die Kamera blickt\n"
        "  --fov <grad>       vertikaler Oeffnungswinkel\n"
        "  --light x y z      Position der Punktlichtquelle\n"
        "  --sky <wert>       Helligkeit des Himmels (0 = schwarzer Hintergrund)\n"
        "  --exposure <wert>  Helligkeitsfaktor fuer das fertige Bild (Standard 1, Kugelszene mit --pt 0.25)\n"
        "  --help             diese Hilfe\n");
}

static bool parse_float(const char *text, float &value) {
    char *end = nullptr;
    value = strtof(text, &end);
    return end != text && *end == '\0';
}

static bool parse_int(const char *text, int &value) {
    char *end = nullptr;
    value = int(strtol(text, &end, 10));
    return end != text && *end == '\0';
}

// Die Szene aus Kugeln: Cornell-Box aus grossen Kugeln, Spiegel, Glas, glossy Gold und eine leuchtende Kugel
static void build_sphere_scene(World* world) {
    WorldObject obj1 = WorldObject({ { 10021.f, 0.0f, 0.f }, 10000.f },{{0.01f, 1.f, 0.01f}, 0});
    WorldObject obj2 = WorldObject({ { -10021.f, 0.0f, 0.f }, 10000.f }, {{1.f, 0.f, 0.f}, 0});
    WorldObject obj4 = WorldObject({ { 0.f, -10012.0f, 0.f }, 10000.f }, {{1.f, 1.f, 1.f}, 0});
    WorldObject obj5 = WorldObject({ { 0.f, 10012.0f, 0.f }, 10000.f }, {{1.f, 1.f, 1.f}, 0});
    WorldObject obj3 = WorldObject({ { 0.0f, 0.0f, -10030.f }, 10000.f }, {{1.f, 1.f, 1.f}, 0});

    WorldObject obj6 = WorldObject({ {3.f, -8.f, -15.f}, 1.f }, {{0.5f, 0.5f, 1.f}, 0});
    WorldObject obj7 = WorldObject({ {-14.f, -8.f, -17.f}, 3.f }, {{1.f, 1.f, 1.f}, 1});

    WorldObject obj8 = WorldObject({ {-3.5f, -9.f, -13.5f}, 3.f }, {{1.f, 1.f, 1.f}, GLASS, 1.5f});            // Glas
    WorldObject obj9 = WorldObject({ {7.5f, -9.5f, -16.f}, 2.5f }, {{0.9f, 0.7f, 0.3f}, GLOSSY, 0.25f});       // Gold, leicht unscharf
    WorldObject obj10 = WorldObject({ {-1.f, 8.5f, -22.f}, 2.f }, {{1.f, 0.85f, 0.6f}, EMISSIVE, 4.f});        // leuchtende Kugel

    PointLight light = PointLight({5.f, 11.f, -18.f});

    world->lights.push_back(light);
    world->add(obj6);
    world->add(obj1);
    world->add(obj2);
    world->add(obj3);
    world->add(obj4);
    world->add(obj5);
    world->add(obj7);
    world->add(obj8);
    world->add(obj9);
    world->add(obj10);
}

int main(int argc, char** argv) {
    std::string scene_file;
    std::string output = "raytracer.bmp";
    int width = 0, height = 0;   // 0: nicht angegeben
    int samples = 0, depth = 10, threads = 0;
    float fov = 0.f, sky = -1.f, exposure = -1.f;   // fov 0, sky < 0 und exposure < 0: Standardwert der Szene
    bool path_tracing = false, badouel = false, gpu = false;
    bool own_cam = false, own_look = false, own_light = false;
    Vector3df cam_position = {0.f, 0.f, 0.f}, look_target = {0.f, 0.f, 0.f}, light_position = {0.f, 0.f, 0.f};

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto has = [&](int count) { return i + count < argc; };
        auto read3 = [&](Vector3df &v) {
            return has(3) && parse_float(argv[i + 1], v[0]) && parse_float(argv[i + 2], v[1]) && parse_float(argv[i + 3], v[2]);
        };

        bool ok = true;
        if (arg == "--help") { print_usage(); return 0; }
        else if (arg == "-w" && has(1))         ok = parse_int(argv[++i], width) && width > 0;
        else if (arg == "-h" && has(1))         ok = parse_int(argv[++i], height) && height > 0;
        else if (arg == "-s" && has(1))         ok = parse_int(argv[++i], samples) && samples > 0;
        else if (arg == "-d" && has(1))         ok = parse_int(argv[++i], depth) && depth > 0;
        else if (arg == "-t" && has(1))         ok = parse_int(argv[++i], threads) && threads > 0;
        else if (arg == "-o" && has(1))         output = argv[++i];
        else if (arg == "--fov" && has(1))      ok = parse_float(argv[++i], fov) && fov > 0.f && fov < 180.f;
        else if (arg == "--sky" && has(1))      ok = parse_float(argv[++i], sky) && sky >= 0.f;
        else if (arg == "--exposure" && has(1)) ok = parse_float(argv[++i], exposure) && exposure > 0.f;
        else if (arg == "--pt")                 path_tracing = true;
        else if (arg == "--badouel")            badouel = true;
        else if (arg == "--gpu")                gpu = true;
        else if (arg == "--cam")              { ok = read3(cam_position);   i += 3; own_cam = true; }
        else if (arg == "--look")             { ok = read3(look_target);    i += 3; own_look = true; }
        else if (arg == "--light")            { ok = read3(light_position); i += 3; own_light = true; }
        else if (!arg.empty() && arg[0] != '-' && scene_file.empty()) scene_file = arg;
        else ok = false;

        if (!ok) {
            printf("Invalid argument: %s\n\n", arg.c_str());
            print_usage();
            return 1;
        }
    }

    // Groesse: ohne Angabe 1280 x 720, mit nur einer Seite wird die andere im Verhaeltnis 16:9 berechnet
    if (width == 0 && height == 0) { width = 1280; height = 720; }
    else if (height == 0) height = std::max(1, int(float(width) * 9.f / 16.f));
    else if (width == 0) width = std::max(1, int(float(height) * 16.f / 9.f));
    if (samples == 0) samples = path_tracing ? 64 : 1;

    auto world = new World();
    auto win = new Window("Raytracer", float(width), float(height), world);
    win->samples_per_pixel = samples;
    win->max_depth = depth;
    win->use_gpu = gpu;
    win->threads = unsigned(threads);
    win->cam->path_tracing = path_tracing;
    world->moeller_trumbore = !badouel;
    win->exposure = exposure > 0.f ? exposure : 1.f;

    if (scene_file.empty()) {
        build_sphere_scene(world);
        if (path_tracing && exposure < 0.f)
            win->exposure = 0.25f;   // die weissen Waende haben Albedo 1: ohne Energieverlust waere das Path-Tracing-Bild sonst ueberbelichtet
        world->sky_brightness = sky >= 0.f ? sky : (path_tracing ? 0.15f : 1.f);   // beim Path-Tracing leuchtet der Himmel durch die offene Vorderseite in den Raum
        if (own_light)
            world->lights[0] = PointLight(light_position);
        if (own_cam)
            win->cam->camera_center = cam_position;
        if (own_look)
            win->cam->look_at(look_target);
        if (fov > 0.f)
            win->cam->set_fov(fov);
    } else {
        // Szene aus einer Wavefront-Datei: Kamera wird auf die Bounding Box der Szene ausgerichtet
        Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};
        if (!load_obj(scene_file, *world, bounds_min, bounds_max))
            return 1;

        Vector3df center = 0.5f * (bounds_min + bounds_max);
        Vector3df extent = bounds_max - bounds_min;
        float diagonal = extent.length();

        world->sky_brightness = sky >= 0.f ? sky : 0.f;   // Standard: schwarzer Hintergrund
        world->ray_bias = 1e-4f * diagonal;               // Abstand fuer Folge- und Schattenstrahlen passend zum Massstab der Szene

        float used_fov = fov > 0.f ? fov : 40.f;
        win->cam->set_fov(used_fov);
        float distance = 1.05f * (0.5f * extent[1]) / std::tan(used_fov * float(M_PI) / 360.f);
        win->cam->camera_center = own_cam ? cam_position : Vector3df{center[0], center[1], bounds_max[2] + distance};
        win->cam->look_at(own_look ? look_target : center);

        // Punktlichtquelle: oben, kurz hinter der Vorderkante der Szene (so werden auch die Vorderseiten beleuchtet).
        // Sie liefert auch beim Path-Tracing den rauschfreien Direktanteil, leuchtende Objekte kommen zusaetzlich dazu.
        if (!own_light)
            light_position = {center[0], bounds_max[1] - 0.05f * extent[1], bounds_max[2] - 0.1f * extent[2]};
        world->lights.push_back(PointLight(light_position));
    }

    return win->Run(output.c_str()) == 1 ? 0 : 1;
}
