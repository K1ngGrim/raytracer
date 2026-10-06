// Metal-Kernel des Raytracers (Port von Camera::ray_color, Camera::lambertian, World::find_nearest_object
// und Sphere/Triangle::intersects). Jeder Thread rechnet einen Pixel.
// Die Datei wird beim Konfigurieren von CMake in das Programm eingebettet und zur Laufzeit kompiliert.
#include <metal_stdlib>
using namespace metal;

// ---- Datenlayout, muss genau zu den Strukturen in gpu/metal_renderer.mm passen ----

struct GpuObject {
    float4 p0;      // Kugel: Mittelpunkt (xyz), Radius (w)   Dreieck: Punkt a
    float4 p1;      // Dreieck: Punkt b
    float4 p2;      // Dreieck: Punkt c
    float4 n0;      // Dreieck: Vertexnormalen von a, b, c
    float4 n1;
    float4 n2;
    float4 color;   // Materialfarbe (xyz), Form (w): 0 = Kugel, 1 = Dreieck
    float4 mat;     // Materialtyp, roughness, refractionIndex, emission
};

struct GpuLight {
    float4 position;
    float4 color;
};

struct Params {
    float4 camera_center;
    float4 pixel00;
    float4 delta_u;
    float4 delta_v;
    uint width;
    uint height;
    uint samples;
    uint max_depth;
    uint num_objects;
    uint num_lights;
    uint path_tracing;
    uint row_start;
    float ray_bias;
    float sky_brightness;
    float exposure;
    float pad;
};

// Materialtypen wie in world/objects/material.h
constant int LAMBERTIAN = 0;
constant int REFLECTIVE = 1;
constant int GLOSSY = 2;
constant int GLASS = 3;
constant int EMISSIVE = 4;

constant int STACK_SIZE = 32;

// ---- Zufallszahlen: gleicher xorshift32 wie math/math.h, damit die Pfade zur CPU-Variante passen ----

inline uint seed_state(uint seed) {
    seed = (seed ^ 61u) ^ (seed >> 16);
    seed *= 9u;
    seed ^= seed >> 4;
    seed *= 0x27d4eb2du;
    seed ^= seed >> 15;
    return seed != 0u ? seed : 1u;
}

inline float random_float(thread uint &state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return float(state >> 8) * (1.0f / 16777216.0f);
}

inline float3 random_in_unit_sphere(thread uint &state) {
    for (int i = 0; i < 64; ++i) {
        float x = -1.0f + 2.0f * random_float(state);
        float y = -1.0f + 2.0f * random_float(state);
        float z = -1.0f + 2.0f * random_float(state);
        float3 p = float3(x, y, z);
        float l = length(p);
        if (l * l < 1.0f)
            return p;
    }
    return float3(0.0f, 0.0f, 1.0f);
}

inline float3 random_unit_vector(thread uint &state) {
    float3 p = random_in_unit_sphere(state);
    return p / length(p);
}

// ---- Schnittpunkte ----

inline bool sphere_inside(float4 sphere, float3 p) {
    return length(sphere.xyz - p) <= sphere.w;
}

// wie Sphere::intersects(ray): Parameter t des Schnittpunkts, 0 wenn es keinen gibt
inline float sphere_t(float4 sphere, float3 o, float3 d) {
    float3 om = o - sphere.xyz;
    float a = dot(d, d);
    float b = 2.0f * dot(om, d);
    float c = dot(om, om) - sphere.w * sphere.w;
    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f)
        return 0.0f;
    disc = sqrt(disc);
    if (sphere_inside(sphere, o))
        return 0.5f * max(-b + disc, -b - disc) / a;
    return 0.5f * min(max(0.0f, -b + disc), -b - disc) / a;
}

// Moeller-Trumbore, wie Triangle::intersects_moeller_trumbore
// wa und wb sind die baryzentrischen Gewichte von a und b
inline bool triangle_t(device const GpuObject &obj, float3 o, float3 d, thread float &t, thread float &wa, thread float &wb) {
    float3 a = obj.p0.xyz;
    float3 edge1 = obj.p1.xyz - a;
    float3 edge2 = obj.p2.xyz - a;

    float3 p_vec = cross(d, edge2);
    float det = dot(edge1, p_vec);
    if (fabs(det) < 1e-8f)
        return false;
    float inv_det = 1.0f / det;

    float3 t_vec = o - a;
    float bary_b = dot(t_vec, p_vec) * inv_det;
    if (bary_b < 0.0f || bary_b > 1.0f)
        return false;

    float3 q_vec = cross(t_vec, edge1);
    float bary_c = dot(d, q_vec) * inv_det;
    if (bary_c < 0.0f || bary_b + bary_c > 1.0f)
        return false;

    t = dot(edge2, q_vec) * inv_det;
    if (t < 0.0f)
        return false;

    wa = 1.0f - bary_b - bary_c;
    wb = bary_b;
    return true;
}

struct Hit {
    int index;
    float t;
    float3 position;
    float3 normal;   // normiert, zeigt gegen den Strahl
};

// Brute-Force-Suche nach dem naechsten Objekt, wie World::find_nearest_object
inline bool find_nearest(device const GpuObject *objs, uint count, float3 o, float3 d, thread Hit &hit) {
    float best = FLT_MAX;
    int index = -1;
    float best_u = 0.0f, best_v = 0.0f;

    for (uint i = 0; i < count; ++i) {
        if (objs[i].color.w < 0.5f) {
            float t = sphere_t(objs[i].p0, o, d);
            if (t > 0.0f && t < best) {
                best = t;
                index = int(i);
            }
        } else {
            float t, wa, wb;
            if (triangle_t(objs[i], o, d, t, wa, wb) && t > 0.0f && t < best) {
                best = t;
                index = int(i);
                best_u = wa;
                best_v = wb;
            }
        }
    }
    if (index < 0)
        return false;

    hit.index = index;
    hit.t = best;
    hit.position = o + best * d;

    device const GpuObject &obj = objs[index];
    if (obj.color.w < 0.5f) {
        float3 n = normalize(hit.position - obj.p0.xyz);
        if (sphere_inside(obj.p0, o))
            n = -n;
        hit.normal = n;
    } else {
        // wie WorldObject::shading_normal: Vertexnormalen interpolieren (u: Gewicht von a, v: Gewicht von b)
        float3 geometric = normalize(cross(obj.p1.xyz - obj.p0.xyz, obj.p2.xyz - obj.p0.xyz));
        float3 n = best_u * obj.n0.xyz + best_v * obj.n1.xyz + (1.0f - best_u - best_v) * obj.n2.xyz;
        if (dot(n, n) < 1e-12f)
            n = geometric;
        n = normalize(n);
        if (dot(n, geometric) < 0.0f)
            n = -n;
        if (dot(geometric, d) > 0.0f)
            n = -n;
        hit.normal = n;
    }
    return true;
}

// true, wenn der Strahl im Inneren des Objekts verlaeuft (wie WorldObject::ray_starts_inside)
inline bool ray_starts_inside(device const GpuObject &obj, float3 o, float3 d) {
    if (obj.color.w < 0.5f)
        return sphere_inside(obj.p0, o);
    float3 outward = cross(obj.p1.xyz - obj.p0.xyz, obj.p2.xyz - obj.p0.xyz);
    return dot(outward, d) > 0.0f;
}

// Liegt zwischen start und start + to_light ein Objekt? (Schattenstrahl, wie in Camera::lambertian)
inline bool in_shadow(device const GpuObject *objs, uint count, float3 start, float3 to_light) {
    for (uint i = 0; i < count; ++i) {
        float t = 0.0f;
        if (objs[i].color.w < 0.5f) {
            t = sphere_t(objs[i].p0, start, to_light);
        } else {
            float wa, wb;
            if (!triangle_t(objs[i], start, to_light, t, wa, wb))
                t = 0.0f;
        }
        if (t > 0.0f && t < 1.0f)
            return true;
    }
    return false;
}

// wie Camera::lambertian
inline float lambertian(device const GpuObject *objs, uint count, float3 position, float3 normal, float3 light_position,
                        float bias, float shadow_intensity) {
    float3 to_light = light_position - position;
    if (in_shadow(objs, count, position + bias * normal, to_light))
        return shadow_intensity;
    return max(0.0f, dot(normal, normalize(to_light)));
}

// Schlick-Naeherung fuer den reflektierten Anteil an Glas
inline float schlick(float cosine, float refraction_index) {
    float r0 = (1.0f - refraction_index) / (1.0f + refraction_index);
    r0 = r0 * r0;
    return r0 + (1.0f - r0) * pow(1.0f - cosine, 5.0f);
}

struct Entry {
    float3 origin;
    float3 direction;
    float3 throughput;
    int depth;
};

// Iterative Version von Camera::ray_color. Die Rekursion wird durch einen Durchsatz (throughput) ersetzt:
// jedes Material multipliziert die Farbe, die der Folgestrahl liefert, mit seiner eigenen Farbe.
// Nur im Whitted-Modus verzweigt Glas in zwei Strahlen, dafuer gibt es den kleinen Stack.
float3 trace(float3 o, float3 d, constant Params &P, device const GpuObject *objs, device const GpuLight *lights,
             thread uint &rng) {
    const bool path_tracing = P.path_tracing != 0u;
    const float bias = P.ray_bias;

    float3 radiance = float3(0.0f);
    float3 throughput = float3(1.0f);
    int depth = int(P.max_depth);
    Entry stack[STACK_SIZE];
    int sp = 0;

    while (true) {
        bool alive = false;   // true: mit (o, d, throughput, depth) wird weitergestrahlt

        if (depth > 0) {
            Hit hit;
            if (!find_nearest(objs, P.num_objects, o, d, hit)) {
                float3 unit_direction = normalize(d);
                float a = 0.5f * (unit_direction.y + 1.0f);
                radiance += throughput * (P.sky_brightness * ((1.0f - a) * float3(1.0f) + a * float3(0.5f, 0.7f, 1.0f)));
            } else {
                device const GpuObject &obj = objs[hit.index];
                float3 n = hit.normal;
                float3 incoming = normalize(d);
                float3 reflection = incoming - 2.0f * dot(incoming, n) * n;
                float3 color = obj.color.xyz;
                int type = int(obj.mat.x);

                if (type == REFLECTIVE) {
                    o = hit.position + (10.0f * bias) * n;
                    d = reflection;
                    throughput *= color;
                    depth -= 1;
                    alive = true;
                } else if (type == GLOSSY) {
                    float3 scattered = reflection;
                    if (path_tracing)
                        scattered = reflection + obj.mat.y * random_unit_vector(rng);
                    if (dot(scattered, n) > 0.0f) {   // sonst in die Oberflaeche gestreut: absorbiert
                        o = hit.position + bias * n;
                        d = scattered;
                        throughput *= color;
                        depth -= 1;
                        alive = true;
                    }
                } else if (type == GLASS) {
                    float refraction_index = obj.mat.z;
                    float eta = ray_starts_inside(obj, o, d) ? refraction_index : 1.0f / refraction_index;
                    float cos_theta = min(-dot(incoming, n), 1.0f);

                    float cos_t = dot(incoming, n);
                    float sin_phi_squared = eta * eta * (1.0f - cos_t * cos_t);
                    bool refracted = sin_phi_squared <= 1.0f;   // sonst Totalreflexion

                    float3 reflect_origin = hit.position + bias * n;
                    if (!refracted) {
                        o = reflect_origin;
                        d = reflection;
                        throughput *= color;
                        depth -= 1;
                        alive = true;
                    } else {
                        float cos_phi = sqrt(1.0f - sin_phi_squared);
                        float3 refraction = eta * (incoming - cos_t * n) - cos_phi * n;
                        float3 refract_origin = hit.position - bias * n;
                        float reflectance = schlick(cos_theta, refraction_index);

                        if (path_tracing) {
                            // zufaellig reflektieren oder brechen, Wahrscheinlichkeit nach Fresnel
                            if (reflectance > random_float(rng)) {
                                o = reflect_origin;
                                d = reflection;
                            } else {
                                o = refract_origin;
                                d = refraction;
                            }
                            throughput *= color;
                            depth -= 1;
                            alive = true;
                        } else {
                            // Whitted: beide Strahlen verfolgen, den reflektierten merken wir uns auf dem Stack
                            if (sp < STACK_SIZE) {
                                stack[sp].origin = reflect_origin;
                                stack[sp].direction = reflection;
                                stack[sp].throughput = throughput * reflectance * color;
                                stack[sp].depth = depth - 1;
                                ++sp;
                            }
                            o = refract_origin;
                            d = refraction;
                            throughput *= (1.0f - reflectance) * color;
                            depth -= 1;
                            alive = true;
                        }
                    }
                } else if (type == EMISSIVE) {
                    radiance += throughput * (obj.mat.w * color);
                } else {
                    // direktes Licht: Lambertian-Shading mit Schattenstrahl, Summe ueber alle Lichter geteilt durch die Anzahl
                    float shadow_intensity = path_tracing ? 0.0f : 0.1f;
                    float3 direct = float3(0.0f);
                    for (uint l = 0; l < P.num_lights; ++l) {
                        float intensity = lambertian(objs, P.num_objects, hit.position, n, lights[l].position.xyz, bias, shadow_intensity);
                        direct += (intensity * color) * lights[l].color.xyz;
                    }
                    if (P.num_lights > 0u)
                        direct = direct / float(P.num_lights);
                    radiance += throughput * direct;

                    if (path_tracing) {
                        // indirektes Licht: ein zufaelliger Strahl, kosinusverteilt um die Normale
                        float3 bounce = n + random_unit_vector(rng);
                        if (dot(bounce, bounce) < 1e-8f)
                            bounce = n;

                        // Russian Roulette, wie auf der CPU
                        float survival = 1.0f;
                        bool survives = true;
                        if (depth <= 7) {
                            survival = max(0.2f, min(0.95f, max(color.x, max(color.y, color.z))));
                            survives = !(random_float(rng) > survival);
                        }
                        if (survives) {
                            o = hit.position + bias * n;
                            d = bounce;
                            throughput *= color / survival;
                            depth -= 1;
                            alive = true;
                        }
                    }
                }
            }
        }

        if (!alive) {
            if (sp == 0)
                break;
            --sp;
            o = stack[sp].origin;
            d = stack[sp].direction;
            throughput = stack[sp].throughput;
            depth = stack[sp].depth;
        }
    }
    return radiance;
}

// wie render_pixel (utility/color.cc): auf 0..1 begrenzen (NaN wird zu 0), mit 255 multiplizieren, Nachkommaanteil verwerfen
inline uint to_channel(float c) {
    c = (c > 0.0f) ? (c < 1.0f ? c : 1.0f) : 0.0f;
    return uint(c * 255.0f);
}

kernel void render(constant Params &P [[buffer(0)]],
                   device const GpuObject *objs [[buffer(1)]],
                   device const GpuLight *lights [[buffer(2)]],
                   device uint *out [[buffer(3)]],   // 0x00RRGGBB, wie Window::pixels
                   uint2 gid [[thread_position_in_grid]]) {
    uint i = gid.x;
    uint j = P.row_start + gid.y;
    if (i >= P.width || j >= P.height)
        return;

    uint rng = seed_state(j * P.width + i);   // pro Pixel, wie in Window::Render

    float3 sum = float3(0.0f);
    for (uint s = 0; s < P.samples; ++s) {
        float di = 0.0f, dj = 0.0f;
        if (P.samples > 1u) {   // bei mehreren Samples zufaellig im Pixel
            di = random_float(rng) - 0.5f;
            dj = random_float(rng) - 0.5f;
        }
        float3 pixel_center = P.pixel00.xyz + ((float(i) + di) * P.delta_u.xyz) + ((float(j) + dj) * P.delta_v.xyz);
        float3 direction = pixel_center - P.camera_center.xyz;
        sum += trace(P.camera_center.xyz, direction, P, objs, lights, rng);
    }

    float3 color = (P.exposure / float(P.samples)) * sum;
    if (P.path_tracing != 0u)   // Gamma-Korrektur (gamma 2)
        color = sqrt(max(color, float3(0.0f)));
    out[j * P.width + i] = (to_channel(color.x) << 16) | (to_channel(color.y) << 8) | to_channel(color.z);
}
