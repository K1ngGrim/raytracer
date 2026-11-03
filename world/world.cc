#include "world.h"

World::World()
= default;

void World::add(WorldObject s) 
{
    this->objects.push_back(s);
}

bool World::ray_intersects_any(Ray3df ray, float max_t) {
    Intersection_Context<float, 3> context{};
    ++this->any_queries;

    for(size_t i = 0; i < this->objects.size(); ++i) {
        const WorldObject & obj = this->objects[i];
        bool intersects;
        if(obj.is_triangle) {
            if(this->moeller_trumbore)
                intersects = obj.triangle.intersects_moeller_trumbore(ray, context);
            else
                intersects = obj.triangle.intersects(ray, context);
        } else {
            intersects = obj.sphere.intersects(ray, context);
        }

        if(intersects && context.t > 0.f && context.t < max_t) {
            this->intersection_tests += i + 1;
            return true;
        }
    }
    this->intersection_tests += this->objects.size();
    return false;
}

WorldObject World::get(int index) 
{
    return this->objects[index];
}

WorldObject* World::find_nearest_object(Ray3df ray, Intersection_Context<float, 3> & fc) {
    Intersection_Context<float, 3> context;
    float t = std::numeric_limits<float>::max();

    if(this->objects.empty()) return nullptr;
    ++this->nearest_queries;
    this->intersection_tests += this->objects.size();

    WorldObject* nearest = nullptr;
    for(WorldObject & obj: this->objects) {
        bool intersects;
        if(obj.is_triangle) {
            if(this->moeller_trumbore)
                intersects = obj.triangle.intersects_moeller_trumbore(ray, context);
            else
                intersects = obj.triangle.intersects(ray, context);
        } else {
            intersects = obj.sphere.intersects(ray, context);
        }

        if(intersects) {
            if(context.t < t && context.t > 0) {
                t = context.t;
                fc = context;
                nearest = &obj;
            }
        }
    }

    if(nearest == nullptr)
        return nullptr;

    // erst fuer das naechste Objekt die Normale bestimmen
    fc.normal = nearest->shading_normal(ray, fc);
    return nearest;
}

WorldObject::WorldObject(Sphere3df sphere, Material c) {
    this->material = c;
    this->sphere = sphere;
}

WorldObject::WorldObject() = default;

WorldObject::WorldObject(Triangle3df triangle, Material c) : triangle(triangle) {
    this->material = c;
    this->is_triangle = true;
}

Vector3df WorldObject::shading_normal(Ray3df ray, Intersection_Context<float, 3> context) const {
    if(!this->is_triangle)
        return context.normal;   // Sphere::intersects liefert die normierte Normale, die gegen den Strahl zeigt

    Vector3df geometric = context.normal;   // (b - a) x (c - a)
    geometric.normalize();

    // Vertexnormalen mit den baryzentrischen Koordinaten interpolieren (u: Gewicht von a, v: Gewicht von b)
    Vector3df normal = context.u * this->triangle.get_na() + context.v * this->triangle.get_nb()
                       + (1.f - context.u - context.v) * this->triangle.get_nc();
    if(normal.square_of_length() < 1e-12f)
        normal = geometric;
    normal.normalize();

    if(normal * geometric < 0.f)   // Vertexnormalen und Dreiecksorientierung muessen zusammenpassen
        normal = -1.f * normal;
    if(geometric * ray.direction > 0.f)   // Normale zeigt gegen den Strahl
        normal = -1.f * normal;
    return normal;
}

bool WorldObject::ray_starts_inside(Ray3df ray) const {
    if(!this->is_triangle)
        return this->sphere.inside(ray.origin);

    // die Dreiecke eines geschlossenen Koerpers sind so orientiert, dass (b - a) x (c - a) nach aussen zeigt
    Vector3df outward = (this->triangle.get_b() - this->triangle.get_a()).cross_product(this->triangle.get_c() - this->triangle.get_a());
    return outward * ray.direction > 0.f;
}
