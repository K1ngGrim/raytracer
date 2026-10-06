#ifndef BMP_H
#define BMP_H

#include <cstdint>
#include <vector>

// Schreibt ein Bild als unkomprimierte 24-Bit-BMP-Datei.
// pixels enthaelt width * height Werte der Form 0x00RRGGBB, Zeile fuer Zeile von oben links.
// Gibt false zurueck, wenn die Groesse nicht passt oder die Datei nicht geschrieben werden konnte.
bool save_bmp(const char *filename, int width, int height, const std::vector<uint32_t> &pixels);

#endif
