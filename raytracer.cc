#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include "io/bmp.h"
#include "io/obj_loader.h"
#include "render/camera.h"
#include "render/renderer.h"
#include "scene/scene.h"

// Cornell-Box aus fuenf grossen Kugeln als Waende, dazu eine diffuse und eine spiegelnde Kugel
static void build_sphere_box(Scene &scene) {
    scene.add(std::make_unique<SphereObject>(Sphere3df({10021.f, 0.f, 0.f}, 10000.f), Material::matte({0.01f, 1.f, 0.01f})));
    scene.add(std::make_unique<SphereObject>(Sphere3df({-10021.f, 0.f, 0.f}, 10000.f), Material::matte({1.f, 0.f, 0.f})));
    scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, 0.f, -10030.f}, 10000.f), Material::matte({1.f, 1.f, 1.f})));
    scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, -10012.f, 0.f}, 10000.f), Material::matte({1.f, 1.f, 1.f})));
    scene.add(std::make_unique<SphereObject>(Sphere3df({0.f, 10012.f, 0.f}, 10000.f), Material::matte({1.f, 1.f, 1.f})));
    scene.add(std::make_unique<SphereObject>(Sphere3df({3.f, -8.f, -15.f}, 1.f), Material::matte({0.5f, 0.5f, 1.f})));
    scene.add(std::make_unique<SphereObject>(Sphere3df({-14.f, -8.f, -17.f}, 3.f), Material::mirror({1.f, 1.f, 1.f})));
    scene.lights.push_back(PointLight({5.f, 11.f, -18.f}));
}

// Kamera und Lichtquelle gehoeren nicht zu Wavefront und werden aus dem Quader um die Szene bestimmt.
// Sie haengen damit nur von der Szene ab und bleiben fuer alle Zeitmessungen gleich.
static void place_camera_and_light(Scene &scene, Camera &camera, Vector3df bounds_min, Vector3df bounds_max) {
    Vector3df center = 0.5f * (bounds_min + bounds_max);
    Vector3df extent = bounds_max - bounds_min;
    float radius = 0.5f * std::max({extent[0], extent[1], extent[2]});
    scene.ray_bias = 1e-4f * extent.length();

    // Kamera: von der Seite mit negativem z, 30 Grad ueber der Horizontalen, im Abstand, in dem die Szene ins Bild passt.
    // In der Testszene stehen die drei Toepfe auf dieser Seite vor der Teekanne.
    float fov = 40.f;
    float distance = 1.05f * radius / std::sin(fov * float(M_PI) / 360.f);
    Vector3df direction = {0.f, std::sin(30.f * float(M_PI) / 180.f), -std::cos(30.f * float(M_PI) / 180.f)};
    camera.place(center + distance * direction, center, fov);

    // Lichtquelle: oben, etwas vor und neben der Szene
    PointLight light = PointLight(center + Vector3df{0.3f * radius, 1.2f * radius, -0.6f * radius});
    scene.lights.push_back(light);
    printf("DEBUG: camera (%.3f, %.3f, %.3f), look at (%.3f, %.3f, %.3f), light (%.3f, %.3f, %.3f)\n",
           camera.position()[0], camera.position()[1], camera.position()[2],
           center[0], center[1], center[2], light.position[0], light.position[1], light.position[2]);
}

static double milliseconds_since(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

// Kommandozeile: raytracer [scene.obj] [-w breite] [-h hoehe] [-s sehstrahlen_pro_pixel] [-d tiefe] [--badouel] [-o ausgabe.bmp]
//   ohne scene.obj wird die Szene aus Kugeln gerendert
//   --badouel: Dreiecke mit Badouel statt Moeller-Trumbore schneiden
int main(int argc, char** argv) {
    std::string scene_file;
    std::string output = "raytracer.bmp";
    int width = 1280, height = 720, samples = 1, depth = 5;
    TriangleMethod method = TriangleMethod::MoellerTrumbore;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-w") == 0 && i + 1 < argc)         width = atoi(argv[++i]);
        else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc)    height = atoi(argv[++i]);
        else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc)    samples = atoi(argv[++i]);
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)    depth = atoi(argv[++i]);
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)    output = argv[++i];
        else if (strcmp(argv[i], "--badouel") == 0)             method = TriangleMethod::Badouel;
        else if (argv[i][0] != '-' && scene_file.empty())       scene_file = argv[i];
        else {
            printf("Usage: raytracer [scene.obj] [-w width] [-h height] [-s samples] [-d depth] [--badouel] [-o output.bmp]\n");
            return 1;
        }
    }
    if (width < 1 || height < 1 || samples < 1 || depth < 1 || !bmp_fits(width, height)) {
        printf("Invalid image size or number of samples\n");
        return 1;
    }

    Scene scene;
    Camera camera(width, height);
    if (scene_file.empty()) {
        build_sphere_box(scene);
    } else {
        Vector3df bounds_min = {0.f, 0.f, 0.f}, bounds_max = {0.f, 0.f, 0.f};
        auto load_start = std::chrono::steady_clock::now();
        if (!load_obj(scene_file, scene, method, bounds_min, bounds_max))
            return 1;
        printf("TIME: load %.1f ms\n", milliseconds_since(load_start));
        place_camera_and_light(scene, camera, bounds_min, bounds_max);
    }

    Renderer renderer(scene, camera);
    renderer.samples_per_pixel = samples;
    renderer.max_depth = depth;

    auto render_start = std::chrono::steady_clock::now();
    std::vector<uint32_t> pixels = renderer.render();
    printf("TIME: render %.1f ms\n", milliseconds_since(render_start));

    // Primaerstrahlen gehen alle in die Suche nach dem naechsten Treffer, der Rest davon sind Folgestrahlen
    uint64_t primary = uint64_t(width) * uint64_t(height) * uint64_t(samples);
    printf("STATS: primary %llu, secondary %llu, shadow %llu, tests %llu\n",
           (unsigned long long) primary, (unsigned long long) (scene.nearest_queries - primary),
           (unsigned long long) scene.any_queries, (unsigned long long) scene.intersection_tests);

    if (!save_bmp(output.c_str(), width, height, pixels)) {
        printf("Error writing %s\n", output.c_str());
        return 1;
    }
    printf("DEBUG: Image saved as %s\n", output.c_str());
    return 0;
}
