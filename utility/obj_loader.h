#ifndef OBJ_LOADER_H
#define OBJ_LOADER_H

#include <string>
#include "../world/world.h"

/**
 * @file obj_loader.h
 * @brief Import von Szenen im Wavefront-Format (.obj mit optionaler .mtl-Datei).
 */

/**
 * @brief Liest eine Wavefront-Datei und fuegt alle Dreiecke als WorldObject in die Welt ein.
 *
 * Unterstuetzt werden die Zeilen v, vn und f (in den Formen v, v/vt, v//vn und v/vt/vn, auch mit negativen
 * Indizes). Polygone werden als Faecher in Dreiecke zerlegt. Texturkoordinaten (vt) werden ignoriert.
 * Ohne Vertexnormalen wird die Normale des Dreiecks verwendet.
 *
 * Materialien kommen aus der mit mtllib angegebenen .mtl-Datei und werden mit usemtl ausgewaehlt:
 * bei d < 1 (oder Tr > 0) ist das Material Glas mit dem Brechungsindex Ni, bei illum 3 oder 5 ein Spiegel
 * (Farbe Ks), sonst ist es diffus (Farbe Kd).
 * Ohne Angabe gilt default_material.
 *
 * @param path             Pfad der .obj-Datei
 * @param world            Welt, in die die Dreiecke eingefuegt werden
 * @param bounds_min       kleinste Koordinaten aller Dreiecke (Ausgabe)
 * @param bounds_max       groesste Koordinaten aller Dreiecke (Ausgabe)
 * @param default_material Material fuer Dreiecke ohne usemtl
 * @return false, wenn die Datei nicht gelesen werden konnte oder kein Dreieck enthielt
 */
bool load_obj(const std::string &path, World &world, Vector3df &bounds_min, Vector3df &bounds_max,
              Material default_material = Material({0.8f, 0.8f, 0.8f}, 0));

#endif
