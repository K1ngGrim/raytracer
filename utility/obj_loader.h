#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <string>
#include "../world/world.h"

// Liest eine Wavefront-Datei (.obj) und fuegt alle Dreiecke als WorldObject in die Welt ein.
// Unterstuetzt: v, vn, f (v, v/vt, v//vn, v/vt/vn, negative Indizes, Polygone werden zu Dreiecken zerlegt),
//               mtllib und usemtl mit einer .mtl-Datei (Kd, Ke, Ks, Ni, d/Tr, illum).
// Ohne Vertexnormalen wird die Normale des Dreiecks verwendet, ohne Material default_material.
// Materialzuordnung aus der .mtl-Datei:
//   Ke > 0 -> EMISSIVE, d < 1 -> GLASS (Ni = Brechungsindex), illum 3 oder 5 -> REFLECTIVE (Farbe Ks), sonst LAMBERTIAN (Farbe Kd)
// bounds_min und bounds_max enthalten danach den Quader um alle Dreiecke der Datei.
// Gibt false zurueck, wenn die Datei nicht gelesen werden konnte oder kein Dreieck enthielt.
bool load_obj(const std::string &path, World &world, Vector3df &bounds_min, Vector3df &bounds_max,
              Material default_material = Material({0.8f, 0.8f, 0.8f}, LAMBERTIAN));

#endif
