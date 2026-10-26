#undef main

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
#include "utility/bmp.h"
#include <cstdlib>
#include <cstring>
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

// Kommandozeile: raytracer [scene.obj] [-w breite] [-h hoehe] [-s sehstrahlen_pro_pixel] [--badouel] [-o ausgabe.bmp]
//   ohne scene.obj wird die Szene aus Kugeln gerendert
//   --badouel: Dreiecke mit Badouel statt Moeller-Trumbore schneiden
int main(int argc, char** argv) {
    std::string scene_file;
    std::string output = "raytracer.bmp";
    int width = 1280, height = 720, samples = 1;
    bool badouel = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc)         width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc)    height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)    samples = atoi(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)    output = argv[++i];
        else if (strcmp(argv[i], "--badouel") == 0)             badouel = true;
        else if (argv[i][0] != '-' && scene_file.empty())       scene_file = argv[i];
        else {
            printf("Usage: raytracer [scene.obj] [-w width] [-h height] [-s samples] [--badouel] [-o output.bmp]\n");
            return 1;
        }
    }
    if (width < 1 || height < 1 || samples < 1 || !bmp_fits(width, height)) {
        printf("Invalid image size or number of samples\n");
        return 1;
    }

    auto world = new World();
    auto win = new Window("Raytracer", float(width), float(height), world);
    win->samples_per_pixel = samples;
    world->moeller_trumbore = !badouel;

    if (scene_file.empty()) {
    WorldObject obj1 = WorldObject({ { 10021.f, 0.0f, 0.f }, 10000.f },{{0.01f, 1.f, 0.01f}, 0});
    WorldObject obj2 = WorldObject({ { -10021.f, 0.0f, 0.f }, 10000.f }, {{1.f, 0.f, 0.f}, 0});
    WorldObject obj4 = WorldObject({ { 0.f, -10012.0f, 0.f }, 10000.f }, {{1.f, 1.f, 1.f}, 0});
    WorldObject obj5 = WorldObject({ { 0.f, 10012.0f, 0.f }, 10000.f }, {{1.f, 1.f, 1.f}, 0});
    WorldObject obj3 = WorldObject({ { 0.0f, 0.0f, -10030.f }, 10000.f }, {{1.f, 1.f, 1.f}, 0});

    WorldObject obj6 = WorldObject({ {3.f, -8.f, -15.f}, 1.f }, {{0.5f, 0.5f, 1.f}, 0});
    WorldObject obj7 = WorldObject({ {-14.f, -8.f, -17.f}, 3.f }, {{1.f, 1.f, 1.f}, 1});

    PointLight light = PointLight({5.f, 11.f, -18.f});

    world->lights.push_back(light);
    world->add(obj6);
    world->add(obj1);
    world->add(obj2);
    world->add(obj3);
    world->add(obj4);
    world->add(obj5);
    world->add(obj7);
    } else {
        // Szene aus einer Wavefront-Datei. Kamera und Lichtquelle gehoeren nicht zu Wavefront und werden hier
        // aus dem Quader um die Szene bestimmt: die Kamera blickt wie immer in -z und steht vor der Szene.
        Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};
        if (!load_obj(scene_file, *world, bounds_min, bounds_max))
            return 1;

        Vector3df center = 0.5f * (bounds_min + bounds_max);
        Vector3df extent = bounds_max - bounds_min;
        float radius = 0.5f * std::max({extent[0], extent[1], extent[2]});
        world->ray_bias = 1e-4f * extent.length();

        // Kamera: schraeg von vorne oben (30 Grad ueber der Horizontalen) im Abstand, in dem die Szene ins Bild passt
        float fov = 40.f;
        win->cam->set_fov(fov);
        float distance = 1.05f * radius / std::sin(fov * float(M_PI) / 360.f);
        Vector3df direction = {0.f, std::sin(30.f * float(M_PI) / 180.f), std::cos(30.f * float(M_PI) / 180.f)};
        win->cam->camera_center = center + distance * direction;
        win->cam->look_at(center);

        // Lichtquelle: oben, etwas vor und neben der Szene
        PointLight light = PointLight(center + Vector3df{0.3f * radius, 1.2f * radius, 0.6f * radius});
        world->lights.push_back(light);
    }

    return win->Run(output.c_str()) == 1 ? 0 : 1;
}
