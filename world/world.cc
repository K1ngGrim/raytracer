#include "world.h"

World::World()
= default;

void World::add(WorldObject s) 
{
    this->objects.push_back(s);
}

bool World::ray_intersects_any(Ray3df ray, Intersection_Context<float, 3> context) {

}

WorldObject World::get(int index) 
{
    return this->objects[index];
}

WorldObject* World::find_nearest_object(Ray3df ray, Intersection_Context<float, 3> & fc) {
    Intersection_Context<float, 3> context;
    float t = std::numeric_limits<float>::max();

    if(this->objects.empty()) return nullptr;

    WorldObject nearest;
    for(WorldObject obj: this->objects) {
        if(obj.sphere.intersects(ray, context)) {
            if(context.t < t && context.t > 0) {
                t = context.t;
                fc = context;
                nearest = obj;
            }
        }
    }

    if(t == std::numeric_limits<float>::max())
        return nullptr;

    return new WorldObject(nearest.sphere, nearest.material);
}

WorldObject::WorldObject(Sphere3df sphere, Material c) {
    this->material = c;
    this->sphere = sphere;
}

WorldObject::WorldObject() = default;