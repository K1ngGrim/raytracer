#include "camera.h"
#include <algorithm>
#include <cmath>
#include <iostream>
using namespace std;

// Schlick-Naeherung: Anteil des Lichts, der an einer Glasoberflaeche reflektiert statt gebrochen wird
static float schlick(float cosine, float refraction_index) {
    float r0 = (1.f - refraction_index) / (1.f + refraction_index);
    r0 = r0 * r0;
    return r0 + (1.f - r0) * std::pow(1.f - cosine, 5.f);
}

color Camera::ray_color(Ray3df ray, World* world, int depth) {
    if (depth <= 0)
        return {0.f, 0.f, 0.f};

    Intersection_Context<float, 3> context;
    WorldObject* hit = world->find_nearest_object(ray, context);
    Light light = world->lights[0];

    if(hit != nullptr) {
        auto n_normal = context.normal;   // normiert, zeigt gegen den Strahl
        auto n_incoming = ray.direction;
        n_incoming.normalize();
        auto v_reflection = n_incoming - 2 * (n_incoming * n_normal) * n_normal;
        float bias = world->ray_bias;

        if(hit->material.type == REFLECTIVE) {
            Ray3df reflection_ray = {context.intersection + (10.f * bias * n_normal), v_reflection};
            return multiply(ray_color(reflection_ray, world, depth - 1), hit->material.materialColor);
        }else if(hit->material.type == GLASS) {
            // Verhaeltnis der Brechungsindizes: Luft -> Glas beim Eintritt, Glas -> Luft beim Austritt
            float refraction_index = hit->material.refractionIndex;
            float eta = hit->ray_starts_inside(ray) ? refraction_index : 1.f / refraction_index;

            Ray3df reflection_ray = {context.intersection + (bias * n_normal), v_reflection};
            Vector3df v_refraction = {0.f, 0.f, 0.f};
            if(!refract(eta, n_normal, n_incoming, v_refraction))   // Totalreflexion
                return multiply(ray_color(reflection_ray, world, depth - 1), hit->material.materialColor);

            // beide Strahlen verfolgen und nach Fresnel mischen
            Ray3df refraction_ray = {context.intersection - (bias * n_normal), v_refraction};
            float reflectance = schlick(std::min(-(n_incoming * n_normal), 1.f), refraction_index);
            auto reflected_color = ray_color(reflection_ray, world, depth - 1);
            auto refracted_color = ray_color(refraction_ray, world, depth - 1);
            return multiply(reflectance * reflected_color + (1.f - reflectance) * refracted_color, hit->material.materialColor);
        }else {
            auto intensity = lambertian(world, light, context);
            return multiply(intensity * hit->material.materialColor, light.lightColor);
        }
    }

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

    // ambienter Anteil: gilt ueberall, auch im Schatten und auf der vom Licht abgewandten Seite.
    // So gibt es keinen Sprung zwischen beschatteten und abgewandten Flaechen.
    const float ambient = 0.1f;

    Vector3df toLight = (light.center - context.intersection);
    Vector3df n_toLight = toLight;
    n_toLight.normalize();
    float cos_theta = context.normal * n_toLight;
    if(cos_theta <= 0.f)
        return ambient;   // Flaeche zeigt vom Licht weg, kein Schattenstrahl noetig

    // Schattenstrahl: toLight ist nicht normiert, das Licht liegt also bei t = 1
    Ray3df shadowRay = {context.intersection + (world->ray_bias * context.normal), toLight};
    if(world->ray_intersects_any(shadowRay, 1.f))
        return ambient;

    return ambient + (1.f - ambient) * cos_theta;
}
