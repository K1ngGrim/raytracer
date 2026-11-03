# Raytracer

Whitted-Raytracer in C++20 für die Rechnerübung zur Vorlesung Optimierung (HKA, Prof. Dr. Christian Pape). Der Raytracer liest Szenen im Wavefront-Format (`.obj` und `.mtl`), berechnet Schatten, Spiegelung und Brechung und schreibt das Bild als BMP-Datei. Für Dreiecke stehen zwei Schnitttests zur Wahl: Möller-Trumbore (Voreinstellung) und Badouel.

## Bauen

Voraussetzungen sind CMake ab 3.9, ein C++20-Compiler und GoogleTest für die Tests.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build -j
```

Ohne Angabe von `CMAKE_BUILD_TYPE` wird ebenfalls im Release-Modus gebaut.

## Aufruf

```
raytracer [szene.obj] [-w breite] [-h hoehe] [-s sehstrahlen] [-d tiefe] [--badouel] [-o ausgabe.bmp]
```

| Parameter | Bedeutung | Voreinstellung |
|---|---|---|
| `szene.obj` | Wavefront-Datei der Szene. Die `.mtl`-Datei wird über `mtllib` relativ zur `.obj` gefunden. Ohne Datei wird eine Box aus Kugeln gerendert. | Box aus Kugeln |
| `-w breite` | Breite des Bildes in Pixeln | 1280 |
| `-h hoehe` | Höhe des Bildes in Pixeln | 720 |
| `-s sehstrahlen` | Sehstrahlen je Pixel. Bei mehr als einem liegen die Strahlen an zufälligen Stellen im Pixel und ihre Farben werden gemittelt (Kantenglättung). | 1 |
| `-d tiefe` | Höchste Rekursionstiefe für Spiegelung und Brechung | 5 |
| `--badouel` | Dreiecke mit Badouel statt mit Möller-Trumbore schneiden | Möller-Trumbore |
| `-o ausgabe.bmp` | Name der Ausgabedatei | `raytracer.bmp` |

Breite, Höhe, Sehstrahlen und Tiefe müssen mindestens 1 sein und die BMP-Datei muss unter 4 GiB bleiben, sonst bricht das Programm mit einer Meldung ab. Ein unbekannter Parameter gibt die Aufrufzeile aus.

Bei einer Wavefront-Szene legt das Programm Kamera und Punktlicht selbst aus dem Quader um alle Dreiecke fest. Die Kamera blickt von der Seite mit negativem *z* aus 30° Höhe mit 40° Öffnungswinkel auf die Mitte der Szene. Das Licht steht über der Szene, etwas seitlich und zur Kamera hin versetzt. Kamera und Licht hängen damit nur von der Szene ab und sind bei allen Messungen gleich.

### Beispiele

Testszene in 320 × 240 Pixeln, wie in der Referenzmessung des Berichts:

```bash
./build/raytracer scenes/teapot_n_glass/teapot_n_glass.obj -w 320 -h 240 -o teapot.bmp
```

Dieselbe Szene mit Badouel:

```bash
./build/raytracer scenes/teapot_n_glass/teapot_n_glass.obj -w 320 -h 240 --badouel -o teapot_badouel.bmp
```

640 × 480 Pixel mit vier Sehstrahlen je Pixel:

```bash
./build/raytracer scenes/teapot_n_glass/teapot_n_glass.obj -w 640 -h 480 -s 4 -o teapot_aa.bmp
```

### Ausgabe

Neben dem Fortschritt gibt das Programm Zeiten und Zähler aus. Das Messskript liest diese Zeilen aus.

```
TIME: load 24.0 ms
DEBUG: camera (0.000, 8.470, -12.021), look at (0.000, 1.529, 0.000), light (1.356, 6.955, -2.713)
TIME: render 16790.0 ms
STATS: primary 76800, secondary 18794, shadow 24398, tests 2132321050
DEBUG: Image saved as teapot.bmp
```

`TIME: load` ist die Zeit für das Einlesen der Szene, `TIME: render` die Zeit für das Rendern ohne Schreiben der Datei. `STATS` zählt Primärstrahlen, Folgestrahlen aus Spiegelung und Brechung, Schattenstrahlen und alle Schnitttests zwischen Strahl und Objekt.

## Wavefront-Import

Gelesen werden `v`, `vn`, `f`, `mtllib` und `usemtl`. Flächen dürfen in den Formen `v`, `v/vt`, `v//vn` und `v/vt/vn` angegeben sein, auch mit negativen Indizes. Polygone werden in Dreiecke zerlegt. Aus der Materialdatei werden diese Werte ausgewertet:

| Wert | Wirkung |
|---|---|
| `Kd` | Farbe diffuser Materialien |
| `Ks` | Farbe spiegelnder Materialien |
| `illum` 3 oder 5 | Material spiegelt |
| `d` < 1 oder `Tr` > 0 | Material ist Glas |
| `Ni` | Brechungsindex von Glas (Voreinstellung 1,5) |

## Tests

Jede Testdatei ist ein eigenes Programm mit GoogleTest: `math_test`, `geometry_test`, `obj_loader_test`, `world_test` und `bmp_test`. Alle zusammen:

```bash
for t in math_test geometry_test obj_loader_test world_test bmp_test; do ./build/$t || break; done
```

## Messungen und Bericht

Das Skript `bericht/messung/messen.sh` baut den Raytracer im Release-Modus und misst alle Konfigurationen des Berichts je dreimal auf der Testszene. Die Ergebnisse landen in `bericht/messung/messungen.csv`, die Bilder in `bericht/bilder/`. Ein Durchlauf dauert rund 45 Minuten.

```bash
sh bericht/messung/messen.sh
```

Der Bericht ist mit Typst gesetzt und berechnet seine Tabellen beim Setzen aus der CSV-Datei.

```bash
typst compile bericht/main.typ bericht/bericht.pdf
```

## Aufbau

| Verzeichnis | Inhalt |
|---|---|
| `math/` | Vektorklasse `Vector<F, N>` |
| `geometry/` | Strahl, Kugel, Dreieck und Box mit ihren Schnitttests |
| `world/` | Szene (`World`), Objekte und Materialien, Suche nach dem nächsten Treffer und Schattentest |
| `view/` | Kamera, Bildebene und Fenster mit der Schleife über alle Pixel |
| `utility/` | Wavefront-Import, BMP-Ausgabe, Farben und Licht |
| `scenes/` | Testszene `teapot_n_glass` |
| `bericht/` | Projektbericht und Messskript |
