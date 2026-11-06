#include "scene.h"
#include <limits>

void Scene::add(std::unique_ptr<SceneObject> object) {
    this->objects.push_back(std::move(object));
}

const SceneObject *Scene::nearest_hit(const Ray3df &ray, Intersection_Context<float, 3> &result) {
    if(this->objects.empty())
        return nullptr;
    ++this->nearest_queries;
    this->intersection_tests += this->objects.size();

    Intersection_Context<float, 3> context;
    float t = std::numeric_limits<float>::max();
    const SceneObject *nearest = nullptr;
    for(const auto &object : this->objects) {
        if(object->intersects(ray, context) && context.t > 0.f && context.t < t) {
            t = context.t;
            result = context;
            nearest = object.get();
        }
    }

    // erst fuer das naechste Objekt die Normale fuer die Beleuchtung bestimmen
    if(nearest != nullptr)
        result.normal = nearest->shading_normal(ray, result);
    return nearest;
}

bool Scene::any_hit(const Ray3df &ray, float max_t) {
    ++this->any_queries;

    Intersection_Context<float, 3> context;
    for(size_t i = 0; i < this->objects.size(); ++i) {
        if(this->objects[i]->intersects(ray, context) && context.t > 0.f && context.t < max_t) {
            this->intersection_tests += i + 1;
            return true;
        }
    }
    this->intersection_tests += this->objects.size();
    return false;
}
