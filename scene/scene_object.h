#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "../geometry/geometry.h"
#include "material.h"
#include <memory>

// Ein Objekt der Szene: Geometrie und Material. Wie ein Strahl die Geometrie schneidet, legen die
// Unterklassen fest. Die Suchen der Szene rufen nur intersects() auf und kennen das Verfahren nicht.
class SceneObject {
public:
    Material material;

    explicit SceneObject(Material material) : material(material) {}
    virtual ~SceneObject() = default;

    // true, wenn der Strahl das Objekt bei t > 0 schneidet. Setzt dann t, den Schnittpunkt,
    // die baryzentrischen Koordinaten u und v (nur Dreiecke) und die geometrische Normale.
    virtual bool intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const = 0;

    // Normale fuer die Beleuchtung, normiert und gegen den Strahl gerichtet
    virtual Vector3df shading_normal(const Ray3df &ray, const Intersection_Context<float, 3> &context) const = 0;

    // true, wenn der Strahl im Inneren des Objekts verlaeuft (bestimmt die Richtung der Brechung)
    virtual bool ray_starts_inside(const Ray3df &ray) const = 0;
};

class SphereObject : public SceneObject {
public:
    SphereObject(Sphere3df sphere, Material material);

    bool intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const override;
    Vector3df shading_normal(const Ray3df &ray, const Intersection_Context<float, 3> &context) const override;
    bool ray_starts_inside(const Ray3df &ray) const override;

private:
    Sphere3df sphere;
};

// Dreieck mit Vertexnormalen. Beleuchtung und Innen/Aussen sind fuer beide Schnittverfahren gleich,
// den Schnitttest selbst implementieren die Unterklassen.
class TriangleObject : public SceneObject {
public:
    TriangleObject(Triangle3df triangle, Material material);

    const Triangle3df &geometry() const { return this->triangle; }

    // interpoliert die Vertexnormalen mit u und v
    Vector3df shading_normal(const Ray3df &ray, const Intersection_Context<float, 3> &context) const override;
    // die Dreiecke eines geschlossenen Koerpers sind so orientiert, dass (b - a) x (c - a) nach aussen zeigt
    bool ray_starts_inside(const Ray3df &ray) const override;

protected:
    Triangle3df triangle;
};

// Schnitt nach Moeller und Trumbore: loest das Gleichungssystem aus Strahl und Kanten direkt nach t, u und v auf
class MoellerTrumboreTriangle : public TriangleObject {
public:
    using TriangleObject::TriangleObject;
    bool intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const override;
};

// Schnitt nach Badouel: erst der Schnittpunkt mit der Ebene, dann der Test im Dreieck in 2D.
// Ebene und Projektionsachsen haengen nur vom Dreieck ab und werden im Konstruktor einmal berechnet.
class BadouelTriangle : public TriangleObject {
public:
    BadouelTriangle(Triangle3df triangle, Material material);
    bool intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const override;

private:
    Vector3df normal = {0.f, 0.f, 0.f};   // (b - a) x (c - a)
    float plane_d = 0.f;                  // Ebene: normal * p + plane_d = 0
    int axis_u = 0, axis_v = 1;           // Achsen der Projektion, die Achse mit der groessten Normalenkomponente faellt weg
};

enum class TriangleMethod { MoellerTrumbore, Badouel };

// erzeugt das Dreieck mit dem gewaehlten Schnittverfahren
std::unique_ptr<TriangleObject> make_triangle(TriangleMethod method, Triangle3df triangle, Material material);

#endif
