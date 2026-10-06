#include "obj_loader.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

namespace {

// Material aus den Werten eines Eintrags der .mtl-Datei
struct MtlEntry {
    color kd = {0.8f, 0.8f, 0.8f};
    color ks = {0.f, 0.f, 0.f};
    color ke = {0.f, 0.f, 0.f};
    float ni = 1.5f;
    float d = 1.f;
    int illum = 2;
};

Material to_material(const MtlEntry &e) {
    if (std::max({e.ke[0], e.ke[1], e.ke[2]}) > 0.f)
        return Material(e.ke, EMISSIVE, 1.f);
    if (e.d < 1.f)
        return Material(e.kd, GLASS, e.ni);
    if (e.illum == 3 || e.illum == 5) {
        bool no_specular = std::max({e.ks[0], e.ks[1], e.ks[2]}) <= 0.f;
        return Material(no_specular ? color{1.f, 1.f, 1.f} : e.ks, REFLECTIVE);
    }
    return Material(e.kd, LAMBERTIAN);
}

std::string directory_of(const std::string &path) {
    size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

void load_mtl(const std::string &path, std::map<std::string, Material> &materials) {
    std::ifstream file(path);
    if (!file) {
        printf("Warning: material file %s not found\n", path.c_str());
        return;
    }

    std::string name;
    MtlEntry entry;
    bool open = false;
    auto finish = [&]() { if (open) materials[name] = to_material(entry); };

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string key;
        in >> key;
        if (key == "newmtl") {
            finish();
            entry = MtlEntry();
            in >> name;
            open = true;
        } else if (key == "Kd") {
            in >> entry.kd[0] >> entry.kd[1] >> entry.kd[2];
        } else if (key == "Ks") {
            in >> entry.ks[0] >> entry.ks[1] >> entry.ks[2];
        } else if (key == "Ke") {
            in >> entry.ke[0] >> entry.ke[1] >> entry.ke[2];
        } else if (key == "Ni") {
            in >> entry.ni;
        } else if (key == "d") {
            in >> entry.d;
        } else if (key == "Tr") {
            float tr = 0.f;
            in >> tr;
            entry.d = 1.f - tr;
        } else if (key == "illum") {
            in >> entry.illum;
        }
    }
    finish();
}

// ein Eintrag einer f-Zeile: Vertex-, Textur- und Normalenindex (0 = nicht angegeben, sonst 1-basiert wie in der Datei)
struct FaceVertex { long v = 0, vt = 0, vn = 0; };

FaceVertex parse_face_vertex(const std::string &token) {
    FaceVertex fv;
    long *targets[3] = {&fv.v, &fv.vt, &fv.vn};
    size_t start = 0;
    for (int k = 0; k < 3 && start <= token.size(); ++k) {
        size_t slash = token.find('/', start);
        std::string part = token.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
        if (!part.empty())
            *targets[k] = std::stol(part);
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return fv;
}

// 1-basierter oder negativer (relativ zum Ende) Index in einen 0-basierten Index, -1 bei ungueltigem Index
long resolve_index(long index, size_t count) {
    if (index > 0 && size_t(index) <= count)
        return index - 1;
    if (index < 0 && size_t(-index) <= count)
        return long(count) + index;
    return -1;
}

}

bool load_obj(const std::string &path, World &world, Vector3df &bounds_min, Vector3df &bounds_max, Material default_material) {
    std::ifstream file(path);
    if (!file) {
        printf("Error: cannot open %s\n", path.c_str());
        return false;
    }

    std::vector<Vector3df> vertices;
    std::vector<Vector3df> normals;
    std::map<std::string, Material> materials;
    Material current = default_material;

    size_t triangle_count = 0, skipped = 0;
    const float inf = std::numeric_limits<float>::max();
    bounds_min = {inf, inf, inf};
    bounds_max = {-inf, -inf, -inf};

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream in(line);
        std::string key;
        in >> key;

        if (key == "v") {
            Vector3df v = {0.f, 0.f, 0.f};
            in >> v[0] >> v[1] >> v[2];
            vertices.push_back(v);
        } else if (key == "vn") {
            Vector3df n = {0.f, 0.f, 0.f};
            in >> n[0] >> n[1] >> n[2];
            normals.push_back(n);
        } else if (key == "mtllib") {
            std::string name;
            in >> name;
            load_mtl(directory_of(path) + name, materials);
        } else if (key == "usemtl") {
            std::string name;
            in >> name;
            auto found = materials.find(name);
            if (found != materials.end()) {
                current = found->second;
            } else {
                printf("Warning: unknown material %s\n", name.c_str());
                current = default_material;
            }
        } else if (key == "f") {
            std::vector<FaceVertex> face;
            std::string token;
            try {
                while (in >> token)
                    face.push_back(parse_face_vertex(token));
            } catch (const std::exception &) {
                ++skipped;
                continue;
            }

            // Polygone als Faecher zerlegen: (0,1,2), (0,2,3), ...
            for (size_t k = 1; k + 1 < face.size(); ++k) {
                const FaceVertex corners[3] = {face[0], face[k], face[k + 1]};
                long vi[3], ni[3];
                bool valid = true, has_normals = true;
                for (int c = 0; c < 3; ++c) {
                    vi[c] = resolve_index(corners[c].v, vertices.size());
                    ni[c] = corners[c].vn != 0 ? resolve_index(corners[c].vn, normals.size()) : -1;
                    if (vi[c] < 0) valid = false;
                    if (ni[c] < 0) has_normals = false;
                }
                if (!valid) { ++skipped; continue; }

                Vector3df a = vertices[vi[0]], b = vertices[vi[1]], c = vertices[vi[2]];
                Vector3df face_normal = (b - a).cross_product(c - a);
                if (face_normal.square_of_length() < 1e-20f) { ++skipped; continue; }   // entartetes Dreieck
                face_normal.normalize();

                Triangle3df triangle = Triangle3df(a, b, c, face_normal);
                if (has_normals) {
                    Vector3df na = normals[ni[0]], nb = normals[ni[1]], nc = normals[ni[2]];
                    if (na.square_of_length() > 0.f && nb.square_of_length() > 0.f && nc.square_of_length() > 0.f) {
                        na.normalize(); nb.normalize(); nc.normalize();
                        triangle = Triangle3df(a, b, c, na, nb, nc);
                    }
                }
                world.add(WorldObject(triangle, current));
                ++triangle_count;

                for (const Vector3df &p : {a, b, c}) {
                    for (size_t axis = 0; axis < 3; ++axis) {
                        bounds_min[axis] = std::min(bounds_min[axis], p[axis]);
                        bounds_max[axis] = std::max(bounds_max[axis], p[axis]);
                    }
                }
            }
        }
    }

    printf("DEBUG: %s: %zu triangles, %zu vertices", path.c_str(), triangle_count, vertices.size());
    if (skipped > 0)
        printf(", %zu faces skipped (invalid indices or degenerate)", skipped);
    printf("\n");
    return triangle_count > 0;
}
