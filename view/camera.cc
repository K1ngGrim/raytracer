#include "camera.h"
#include <algorithm>
#include <iostream>
using namespace std;

// Schlick-Naeherung: Anteil des Lichts, der an einer Glasoberflaeche reflektiert statt gebrochen wird
static float schlick(float cosine, float refraction_index) {
    float r0 = (1.f - refraction_index) / (1.f + refraction_index);
    r0 = r0 * r0;
    return r0 + (1.f - r0) * pow(1.f - cosine, 5.f);
}

color Camera::ray_color(Ray3df ray, World* world, int depth) {
    if (depth <= 0)
        return {0.f, 0.f, 0.f};

    Intersection_Context<float, 3> context;
    WorldObject* hit = world->find_nearest_object(ray, context);

    if(hit != nullptr) {
        auto n_normal = context.normal;   // normiert, zeigt gegen den Strahl
        auto n_incoming = ray.direction;
        n_incoming.normalize();
        auto v_reflection = n_incoming - 2 * (n_incoming * n_normal) * n_normal;
        auto draw_color = (hit->material.materialColor);
        float bias = world->ray_bias;   // Abstand, um den neue Strahlen von der Oberflaeche weg starten

        if(hit->material.type == REFLECTIVE) {
            Ray3df reflection_ray = {context.intersection + (10.f * bias * n_normal), v_reflection};
            return multiply(ray_color(reflection_ray, world, depth - 1), draw_color);
        }else if(hit->material.type == GLOSSY) {
            // wie der Spiegel, aber beim Path-Tracing wird die Richtung je nach roughness zufaellig gestreut
            auto v_scattered = v_reflection;
            if(path_tracing)
                v_scattered = v_reflection + hit->material.roughness * random_unit_vector();
            if(v_scattered * n_normal <= 0.f)
                return {0.f, 0.f, 0.f};   // in die Oberflaeche gestreut: Licht wird absorbiert
            Ray3df scattered_ray = {context.intersection + (bias * n_normal), v_scattered};
            return multiply(ray_color(scattered_ray, world, depth - 1), draw_color);
        }else if(hit->material.type == GLASS) {
            float refraction_index = hit->material.refractionIndex;
            // Verhaeltnis der Brechungsindizes: Luft -> Glas beim Eintritt, Glas -> Luft beim Austritt
            float eta = hit->ray_starts_inside(ray) ? refraction_index : 1.f / refraction_index;
            float cos_theta = std::min(-(n_incoming * n_normal), 1.f);

            Vector3df v_refraction = {0.f, 0.f, 0.f};
            bool refracted = refract(eta, n_normal, n_incoming, v_refraction);
            Ray3df reflection_ray = {context.intersection + (bias * n_normal), v_reflection};
            if(!refracted) // Totalreflexion
                return multiply(ray_color(reflection_ray, world, depth - 1), draw_color);

            Ray3df refraction_ray = {context.intersection - (bias * n_normal), v_refraction};
            float reflectance = schlick(cos_theta, refraction_index);
            if(path_tracing) {
                // Path-Tracing: zufaellig reflektieren oder brechen, Wahrscheinlichkeit nach Fresnel
                if(reflectance > random_float())
                    return multiply(ray_color(reflection_ray, world, depth - 1), draw_color);
                return multiply(ray_color(refraction_ray, world, depth - 1), draw_color);
            }
            // Whitted: beide Strahlen verfolgen und nach Fresnel mischen
            auto reflected_color = ray_color(reflection_ray, world, depth - 1);
            auto refracted_color = ray_color(refraction_ray, world, depth - 1);
            return multiply(reflectance * reflected_color + (1.f - reflectance) * refracted_color, draw_color);
        }else if(hit->material.type == EMISSIVE) {
            return hit->material.emission * draw_color;
        }else {
            // direktes Licht: Lambertian-Shading mit Schattenstrahl, Summe ueber alle Lichter geteilt durch die Anzahl
            float shadow_intensity = path_tracing ? 0.f : 0.1f;
            color direct = {0.f, 0.f, 0.f};
            for(Light & light : world->lights) {
                auto intensity = lambertian(world, light, context, shadow_intensity);
                direct = direct + multiply(intensity * hit->material.materialColor, light.lightColor);
            }
            if(!world->lights.empty())
                direct = direct / float(world->lights.size());

            if(!path_tracing)
                return direct;

            // indirektes Licht: ein zufaelliger Strahl, kosinusverteilt um die Normale
            auto v_bounce = n_normal + random_unit_vector();
            if(v_bounce.square_of_length() < 1e-8f)
                v_bounce = n_normal;
            Ray3df bounce_ray = {context.intersection + (bias * n_normal), v_bounce};

            // Russian Roulette: nach einigen Bounces wird der Pfad zufaellig beendet, abhaengig von der Helligkeit des Materials.
            // Die ueberlebenden Pfade werden entsprechend staerker gewichtet, das Ergebnis bleibt im Mittel gleich.
            float survival = 1.f;
            if(depth <= 7) {
                survival = std::max(0.2f, std::min(0.95f, std::max({draw_color[0], draw_color[1], draw_color[2]})));
                if(random_float() > survival)
                    return direct;
            }
            return direct + multiply(ray_color(bounce_ray, world, depth - 1), draw_color / survival);
        }
    }

    Vector3df unit_direction = ray.direction;
    unit_direction.normalize();
    auto a = 0.5f*(unit_direction[1] + 1.0f);
    return world->sky_brightness * ((1.0f-a)*color{1.f, 1.f, 1.f} + a*color{0.5f, 0.7f, 1.f});
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

float Camera::lambertian(World* world, Light light, Intersection_Context<float, 3> context, float shadow_intensity) {
    //Gegeben Oberflächen normale ~n eines Oberflächenpunkts, Blickrichtung ~v zum
    //Auge, Richtung zur Lichtquelle ~l und Anteil diffusen Lichts kd.
    //Gesucht Intensität des Lichts (Farbe) L, die gesehen wird.
    //L = kd ·~l max{0, cos Θ} = kd ·~l max{0, ~n ·~l}
    //Ben: color * max(0, n * l) alle vektoren normieren

    //erstmal ohne schattenwurf von anderen objekten zwischen obj1 und licht;
    //Vektor zum licht = lichtpunkt - intersection

    Vector3df toLight = (light.center - context.intersection);
    Vector3df newStart = context.intersection + (world->ray_bias * context.normal);
    Ray3df newRay = {newStart, toLight};
    Intersection_Context<float, 3> c{};
    world->find_nearest_object(newRay, c);

    if(c.t > 0.f && c.t < 1.f)
        return shadow_intensity;

    toLight.normalize();

    return (max(0.f, context.normal * toLight));

}



