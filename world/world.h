#ifndef WORLD_H
#define WORLD_H

#include "../geometry/geometry.h"
#include "../utility/color.h"
#include "../utility/light.h"
#include "objects/material.h"

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
};

class World {
public:
    World();

    std::vector<WorldObject> objects;
    std::vector<Light> lights;

    float ray_bias = 0.01f;         // Abstand, um den Schatten- und Folgestrahlen von der Oberflaeche weg starten (abhaengig vom Massstab der Szene)
    bool moeller_trumbore = true;   // Dreiecke: Moeller-Trumbore (true) oder Badouel (false)

    void add(WorldObject s);
    WorldObject get(int index);

    bool ray_intersects_any(Ray3df ray, Intersection_Context<float, 3> context);
    // Brute-Force-Suche ueber alle Objekte. Der Zeiger zeigt in objects (nicht freigeben), nullptr wenn nichts getroffen wurde.
    WorldObject* find_nearest_object(Ray3df ray, Intersection_Context<float, 3> &fc);
};

class LambertianSphere : WorldObject {
public:
};

#endif