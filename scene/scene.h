#ifndef SCENE_H
#define SCENE_H

#include "light.h"
#include "scene_object.h"
#include <cstdint>
#include <memory>
#include <vector>

// Alle Objekte und Lichtquellen sowie die beiden Suchen, die ein Strahl in der Szene braucht.
// Aktuell sind beide Suchen Brute Force ueber alle Objekte.
class Scene {
public:
    std::vector<std::unique_ptr<SceneObject>> objects;
    std::vector<PointLight> lights;

    float ray_bias = 0.01f;   // Abstand, um den Schatten- und Folgestrahlen von der Oberflaeche weg starten (abhaengig vom Massstab der Szene)

    // Zaehler fuer die Auswertung: Aufrufe der beiden Suchen und durchgefuehrte Schnitttests
    uint64_t nearest_queries = 0;
    uint64_t any_queries = 0;
    uint64_t intersection_tests = 0;

    void add(std::unique_ptr<SceneObject> object);

    // naechster Treffer mit t > 0. context.normal ist danach die Normale fuer die Beleuchtung.
    // nullptr, wenn der Strahl nichts trifft.
    const SceneObject *nearest_hit(const Ray3df &ray, Intersection_Context<float, 3> &context);

    // Schattentest: true, sobald irgendein Objekt den Strahl mit 0 < t < max_t schneidet (bricht beim ersten Treffer ab)
    bool any_hit(const Ray3df &ray, float max_t);
};

#endif
