#ifndef BMP_H
#define BMP_H

#include <cstdint>
#include <vector>

// Prueft, ob ein Bild dieser Groesse in eine BMP-Datei passt (das Format speichert Groessen als 32-Bit-Zahlen, die Datei ist maximal 4 GiB gross).
// Gibt false zurueck, wenn es zu gross waere oder die Groesse ungueltig ist.
bool bmp_fits(int width, int height);

// Schreibt ein Bild als unkomprimierte 24-Bit-BMP-Datei.
// pixels enthaelt width * height Werte der Form 0x00RRGGBB, Zeile fuer Zeile von oben links.
// Gibt false zurueck, wenn die Groesse nicht passt oder die Datei nicht geschrieben werden konnte.
bool save_bmp(const char *filename, int width, int height, const std::vector<uint32_t> &pixels);

#endif
