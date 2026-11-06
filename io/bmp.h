#ifndef BMP_H
#define BMP_H

#include <cstdint>
#include <vector>

/**
 * @file bmp.h
 * @brief Schreibt Bilder als unkomprimierte 24-Bit-BMP-Datei, ohne externe Bibliothek.
 */

/**
 * @brief Prueft, ob ein Bild dieser Groesse in eine BMP-Datei passt.
 *
 * Das BMP-Format speichert Groessen als 32-Bit-Zahlen, die Datei darf daher hoechstens 4 GiB gross sein.
 *
 * @param width  Breite in Pixeln
 * @param height Hoehe in Pixeln
 * @return false, wenn das Bild zu gross waere oder die Groesse ungueltig ist
 */
bool bmp_fits(int width, int height);

/**
 * @brief Schreibt ein Bild als BMP-Datei.
 *
 * @param filename Name der Ausgabedatei
 * @param width    Breite in Pixeln
 * @param height   Hoehe in Pixeln
 * @param pixels   width * height Farbwerte der Form 0x00RRGGBB, Zeile fuer Zeile von oben links
 * @return false, wenn die Groesse nicht passt oder die Datei nicht geschrieben werden konnte
 */
bool save_bmp(const char *filename, int width, int height, const std::vector<uint32_t> &pixels);

#endif
