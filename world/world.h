#ifndef WORLD_H
#define WORLD_H

#include "../geometry/geometry.h"
#include "../utility/color.h"
#include "../utility/light.h"
#include "objects/material.h"
#include <cstdint>

class WorldObject {
public:
    WorldObject(Sphere3df sphere, Material c);
    WorldObject(Triangle3df triangle, Material c);
    WorldObject();
    Material material {};
    Sphere3df sphere = Sphere3df({ 0.0f, 0.0f, 0.f }, 0.f);
    Triangle3df triangle = Triangle3df({ 0.0f, 0.0f, 0.f }, { 0.0f, 0.0f, 0.f }, { 0.0f, 0.0f, 0.f });
    bool is_triangle = false;   // false: das Objekt ist die Kugel, true: das Dreieck

    // Normale fuer die Beleuchtung: bei Dreiecken aus den Vertexnormalen interpoliert, normiert und gegen den Strahl gerichtet
    // (wie bei Sphere::intersects), bei Kugeln unveraendert
    Vector3df shading_normal(Ray3df ray, Intersection_Context<float, 3> context) const;

    // true, wenn der Strahl im Inneren des Objekts verlaeuft (Kugel: Startpunkt innen, Dreieck: Strahl verlaesst die Vorderseite)
    bool ray_starts_inside(Ray3df ray) const;
};

class World {
public:
    World();

    std::vector<WorldObject> objects;
    std::vector<Light> lights;

    float ray_bias = 0.01f;         // Abstand, um den Schatten- und Folgestrahlen von der Oberflaeche weg starten (abhaengig vom Massstab der Szene)
    bool moeller_trumbore = true;   // Dreiecke: Moeller-Trumbore (true) oder Badouel (false)

    // Zaehler fuer die Auswertung: Aufrufe der beiden Suchen und durchgefuehrte Schnitttests
    uint64_t nearest_queries = 0;
    uint64_t any_queries = 0;
    uint64_t intersection_tests = 0;

    void add(WorldObject s);
    WorldObject get(int index);

    // Schattentest: true, sobald irgendein Objekt den Strahl mit 0 < t < max_t schneidet (bricht beim ersten Treffer ab)
    bool ray_intersects_any(Ray3df ray, float max_t);
    // Brute-Force-Suche ueber alle Objekte. Der Zeiger zeigt in objects (nicht freigeben), nullptr wenn nichts getroffen wurde.
    WorldObject* find_nearest_object(Ray3df ray, Intersection_Context<float, 3> &fc);
};

class LambertianSphere : WorldObject {
public:
};

#endif