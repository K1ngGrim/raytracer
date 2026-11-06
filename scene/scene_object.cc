#include "scene_object.h"
#include <cmath>

SphereObject::SphereObject(Sphere3df sphere, Material material) : SceneObject(material), sphere(sphere) {}

bool SphereObject::intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const {
    return this->sphere.intersects(ray, context);
}

Vector3df SphereObject::shading_normal(const Ray3df &, const Intersection_Context<float, 3> &context) const {
    return context.normal;   // Sphere::intersects liefert die normierte Normale, die gegen den Strahl zeigt
}

bool SphereObject::ray_starts_inside(const Ray3df &ray) const {
    return this->sphere.inside(ray.origin);
}

TriangleObject::TriangleObject(Triangle3df triangle, Material material) : SceneObject(material), triangle(triangle) {}

Vector3df TriangleObject::shading_normal(const Ray3df &ray, const Intersection_Context<float, 3> &context) const {
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

bool TriangleObject::ray_starts_inside(const Ray3df &ray) const {
    Vector3df outward = (this->triangle.get_b() - this->triangle.get_a()).cross_product(this->triangle.get_c() - this->triangle.get_a());
    return outward * ray.direction > 0.f;
}

bool MoellerTrumboreTriangle::intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const {
    return this->triangle.intersects_moeller_trumbore(ray, context);
}

BadouelTriangle::BadouelTriangle(Triangle3df triangle, Material material) : TriangleObject(triangle, material) {
    this->normal = (triangle.get_b() - triangle.get_a()).cross_product(triangle.get_c() - triangle.get_a());
    this->plane_d = -(this->normal * triangle.get_a());

    // das Dreieck wird auf die Koordinatenebene projiziert, in der es am groessten erscheint
    int dominant = 0;
    for(int axis = 1; axis < 3; ++axis) {
        if(std::fabs(this->normal[axis]) > std::fabs(this->normal[dominant]))
            dominant = axis;
    }
    this->axis_u = dominant == 0 ? 1 : 0;
    this->axis_v = dominant == 2 ? 1 : 2;
}

// Badouel, "An Efficient Ray-Polygon Intersection", Graphics Gems (1990)
bool BadouelTriangle::intersects(const Ray3df &ray, Intersection_Context<float, 3> &context) const {
    const float EPSILON = 10e-9f;

    // Schnittpunkt mit der Ebene des Dreiecks
    float denominator = this->normal * ray.direction;
    if(std::fabs(denominator) < EPSILON)   // Strahl parallel zur Ebene
        return false;
    float t = -(this->plane_d + this->normal * ray.origin) / denominator;
    if(t < 0.f)
        return false;
    Vector3df p = ray.origin + t * ray.direction;

    // Lage des Punkts im projizierten Dreieck: p - a = alpha (b - a) + beta (c - a)
    const Vector3df a = this->triangle.get_a(), b = this->triangle.get_b(), c = this->triangle.get_c();
    float u0 = p[this->axis_u] - a[this->axis_u], v0 = p[this->axis_v] - a[this->axis_v];
    float u1 = b[this->axis_u] - a[this->axis_u], v1 = b[this->axis_v] - a[this->axis_v];
    float u2 = c[this->axis_u] - a[this->axis_u], v2 = c[this->axis_v] - a[this->axis_v];

    float alpha, beta;
    if(u1 == 0.f) {
        beta = u0 / u2;
        if(beta < 0.f || beta > 1.f)
            return false;
        alpha = (v0 - beta * v2) / v1;
    } else {
        beta = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
        if(beta < 0.f || beta > 1.f)
            return false;
        alpha = (u0 - beta * u2) / u1;
    }
    if(alpha < 0.f || alpha + beta > 1.f)
        return false;

    // gleiche Konvention wie Moeller-Trumbore: u ist das Gewicht von a, v das Gewicht von b
    context.t = t;
    context.intersection = p;
    context.u = 1.f - alpha - beta;
    context.v = alpha;
    context.normal = this->normal;
    return true;
}

std::unique_ptr<TriangleObject> make_triangle(TriangleMethod method, Triangle3df triangle, Material material) {
    switch(method) {
        case TriangleMethod::Badouel:
            return std::make_unique<BadouelTriangle>(triangle, material);
        case TriangleMethod::MoellerTrumbore:
        default:
            return std::make_unique<MoellerTrumboreTriangle>(triangle, material);
    }
}
