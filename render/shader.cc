#include "shader.h"
#include <cmath>

color Shader::trace(const Ray3df &ray, int depth) {
    if(depth <= 0)
        return {0.f, 0.f, 0.f};

    Intersection_Context<float, 3> context;
    const SceneObject *hit = this->scene.nearest_hit(ray, context);
    if(hit == nullptr)
        return background(ray);

    const Material &material = hit->material;
    Vector3df normal = context.normal;   // normiert, zeigt gegen den Strahl
    Vector3df direction = ray.direction;
    direction.normalize();
    // d ist die Deckkraft der Oberflaeche: ihre eigene Farbe zaehlt nur mit d, den Rest liefert der gebrochene Strahl
    color result = (1.f - material.transparency) * this->direct_light(material, context.intersection, normal, -1.f * direction);

    float bias = this->scene.ray_bias;
    color reflection_weight = material.specular;

    if(material.transmits()) {
        // Verhaeltnis der Brechungsindizes: Luft -> Glas beim Eintritt, Glas -> Luft beim Austritt
        float eta = hit->ray_starts_inside(ray) ? material.refraction_index : 1.f / material.refraction_index;
        Vector3df refracted = {0.f, 0.f, 0.f};
        if(refract(eta, normal, direction, refracted)) {
            Ray3df refraction_ray = {context.intersection - bias * normal, refracted};
            result = result + material.transparency * this->trace(refraction_ray, depth - 1);
        } else {
            // Totalreflexion: der Anteil, der gebrochen worden waere, wird ebenfalls gespiegelt
            reflection_weight = reflection_weight + color{material.transparency, material.transparency, material.transparency};
        }
    }

    if(reflection_weight[0] > 0.f || reflection_weight[1] > 0.f || reflection_weight[2] > 0.f) {
        Vector3df reflected = direction - 2.f * (direction * normal) * normal;
        Ray3df reflection_ray = {context.intersection + bias * normal, reflected};
        result = result + multiply(reflection_weight, this->trace(reflection_ray, depth - 1));
    }
    return result;
}

color Shader::direct_light(const Material &material, Vector3df point, Vector3df normal, Vector3df to_viewer) {
    // der ambiente Anteil gilt ueberall, auch im Schatten und auf der vom Licht abgewandten Seite
    color result = material.ambient;

    for(const PointLight &light : this->scene.lights) {
        Vector3df to_light = light.position - point;
        Vector3df l = to_light;
        l.normalize();
        float cos_theta = normal * l;
        if(cos_theta <= 0.f)
            continue;   // Flaeche zeigt vom Licht weg, kein Schattenstrahl noetig

        // Schattenstrahl: to_light ist nicht normiert, das Licht liegt also bei t = 1
        Ray3df shadow_ray = {point + this->scene.ray_bias * normal, to_light};
        if(this->scene.any_hit(shadow_ray, 1.f))
            continue;

        color lit = cos_theta * material.diffuse;
        if(material.reflects()) {
            // Glanzlicht nach Phong: Richtung zum Licht an der Normale gespiegelt und mit der Richtung zum Auge verglichen
            Vector3df mirrored = 2.f * cos_theta * normal - l;
            float cos_alpha = mirrored * to_viewer;
            if(cos_alpha > 0.f)
                lit = lit + std::pow(cos_alpha, material.shininess) * material.specular;
        }
        result = result + multiply(lit, light.intensity);
    }
    return result;
}

color Shader::background(const Ray3df &ray) {
    Vector3df unit_direction = ray.direction;
    unit_direction.normalize();
    float a = 0.5f * (unit_direction[1] + 1.f);
    return (1.f - a) * color{1.f, 1.f, 1.f} + a * color{0.5f, 0.7f, 1.f};
}
