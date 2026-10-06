#include "camera.h"
#include <iostream>
using namespace std;

color Camera::ray_color(Ray3df ray, World* world, int depth) {
    Intersection_Context<float, 3> context;
    WorldObject* hit = world->find_nearest_object(ray, context);
    Light light = world->lights[0];

    if (depth <= 0)
        return {0.f, 0.f, 0.f};

    if(hit != nullptr) {
        if(hit->material.type == 1) {
            auto n_normal = context.normal;
            auto n_incoming = ray.direction;
            n_incoming.normalize();
            auto v_reflection = n_incoming - 2 * (n_incoming * n_normal) * n_normal;
            Ray3df reflection_ray = {context.intersection + (0.1f * n_normal), v_reflection};
            auto draw_color = (hit->material.materialColor);
            return multiply(ray_color(reflection_ray, world, depth - 1), draw_color);
        }else {
            auto intensity = lambertian(world, light, context);

            auto n_normal = context.normal; n_normal.normalize();
            auto n_incoming = ray.direction; n_incoming.normalize();
            auto reflection = n_incoming - 2*(n_incoming * n_normal) * n_normal;
            return multiply(intensity * hit->material.materialColor, light.lightColor);

        }
    }

    delete hit;
    Vector3df unit_direction = ray.direction;
    unit_direction.normalize();
    auto a = 0.5f*(unit_direction[1] + 1.0f);
    return (1.0f-a)*color{1.f, 1.f, 1.f} + a*color{0.5f, 0.7f, 1.f};
}

/*
color Camera::cast_ray(Ray3df ray, World* world, int depth) {
    if(depth-- <= 0)
        return {0.f, 0.f, 0.f};

    Intersection_Context<float, 3> context;
    WorldObject* obj = world->find_nearest_object(ray, context);
    Light light = world->lights[0];

    if(obj != nullptr) {
        if(obj->material.type == 1) {
            auto n_normal = context.normal;
            n_normal.normalize();
            auto n_incoming = ray.direction;
            n_incoming.normalize();
            //v - 2*dot(v,n)*n v = incoming, n = normal. Alle vektoren müssen normiert sein
            auto intensity = lambertian(world, light, context);
            auto reflection = n_incoming - 2*(n_incoming * n_normal) * n_normal;
            Ray3df newRay = {context.intersection + (0.1f * n_normal), reflection};
            auto draw_color = (intensity * obj->material.materialColor);
            return multiply(intensity * cast_ray(newRay, world, depth), draw_color);
        }else {
            auto intensity = lambertian(world, light, context);
            auto draw_color = (intensity * light.lightColor);
            draw_color = multiply(draw_color, obj->material.materialColor);

            return draw_color;
        }


    }

    Vector3df unit_direction = ray.direction;
    unit_direction.normalize();
    auto a = 0.5f*(unit_direction[1] + 1.0f);
    return (1.0f-a)*color{1.f, 1.f, 1.f} + a*color{0.5f, 0.7f, 1.f};
}*/

/*
 * color WorldObject::getColor(World* world, Intersection_Context<float, 3> & context, int depth) {
    auto intensity = (this->lambertian(world, world->lights[0], context));

    Vector3df toLight = (light.center - context.intersection);
    Vector3df newStart = context.intersection + (0.004f * context.normal);
    Ray3df newRay = {newStart, toLight};
    Intersection_Context<float, 3> c;
    world->find_nearest_object(newRay, c);

    if(c.t < 1.f)
        return 0.01f;

    toLight.normalize();
    Vector3df n = context.normal;
    n.normalize();

    return (max(0.f, n * toLight));

    auto draw_color = intensity * world->lights[0].lightColor;
    for(size_t t = 0u; t < 3u; t++) {
        draw_color[t] *= obj->material[t];
    }
    return draw_color;
}
 */

float Camera::lambertian(World* world, Light light, Intersection_Context<float, 3> context) {
    //Gegeben Oberflächen normale ~n eines Oberflächenpunkts, Blickrichtung ~v zum
    //Auge, Richtung zur Lichtquelle ~l und Anteil diffusen Lichts kd.
    //Gesucht Intensität des Lichts (Farbe) L, die gesehen wird.
    //L = kd ·~l max{0, cos Θ} = kd ·~l max{0, ~n ·~l}
    //Ben: color * max(0, n * l) alle vektoren normieren

    //erstmal ohne schattenwurf von anderen objekten zwischen obj1 und licht;
    //Vektor zum licht = lichtpunkt - intersection

    Vector3df toLight = (light.center - context.intersection);
    Vector3df newStart = context.intersection + (0.01f * context.normal);
    Ray3df newRay = {newStart, toLight};
    Intersection_Context<float, 3> c;
    world->find_nearest_object(newRay, c);

    if(c.t > 0.f && c.t < 1.f)
        return 0.1f;

    toLight.normalize();

    return (max(0.f, context.normal * toLight));

}



