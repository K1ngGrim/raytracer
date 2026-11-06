# Raytracer

Whitted-Raytracer in C++20 für die Rechnerübung zur Vorlesung Optimierung (HKA, Prof. Dr. Christian Pape). Der Raytracer liest Szenen im Wavefront-Format (`.obj` und `.mtl`), berechnet Schatten, Spiegelung und Brechung und schreibt das Bild als BMP-Datei. Für Dreiecke stehen zwei Schnitttests zur Wahl: Möller-Trumbore (Voreinstellung) und Badouel.

## Bauen

Voraussetzungen sind CMake ab 3.10, ein C++20-Compiler und GoogleTest für die Tests.

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
TIME: load 22.5 ms
DEBUG: camera (0.000, 8.470, -12.021), look at (0.000, 1.529, 0.000), light (1.356, 6.955, -2.713)
TIME: render 18645.3 ms
STATS: primary 76800, secondary 20988, shadow 37246, tests 2312686972
DEBUG: Image saved as teapot.bmp
```

`TIME: load` ist die Zeit für das Einlesen der Szene, `TIME: render` die Zeit für das Rendern ohne Schreiben der Datei. `STATS` zählt Primärstrahlen, Folgestrahlen aus Spiegelung und Brechung, Schattenstrahlen und alle Schnitttests zwischen Strahl und Objekt.

## Wavefront-Import

Gelesen werden `v`, `vn`, `f`, `mtllib` und `usemtl`. Flächen dürfen in den Formen `v`, `v/vt`, `v//vn` und `v/vt/vn` angegeben sein, auch mit negativen Indizes. Polygone werden in Dreiecke zerlegt. Das Schnittverfahren (`--badouel`) wird beim Einlesen festgelegt, jedes Dreieck ist danach ein Objekt der passenden Unterklasse. Aus der Materialdatei werden diese Werte ausgewertet:

| Wert | Wirkung |
|---|---|
| `Ka` | ambiente Farbe, gilt überall (Voreinstellung 0,1) |
| `Kd` | diffuse Farbe nach Lambert |
| `Ks` | Stärke des Glanzlichts nach Phong und Gewicht des gespiegelten Strahls |
| `Ns` | Exponent des Glanzlichts |
| `d` oder `Tr` | Deckkraft der Oberfläche: `Ka`, `Kd` und Glanzlicht zählen mit `d`, der gebrochene Strahl mit `1 - d` |
| `Ni` | Brechungsindex (Voreinstellung 1,5) |

`illum` wird nicht ausgewertet. Ob ein Material spiegelt, ergibt sich allein aus `Ks`.

## Tests

Jede Testdatei ist ein eigenes Programm mit GoogleTest: `math_test`, `geometry_test`, `scene_test`, `render_test`, `obj_loader_test` und `bmp_test`. Alle zusammen:

```bash
for t in math_test geometry_test scene_test render_test obj_loader_test bmp_test; do ./build/$t || break; done
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
| `math/` | Vektorklasse `Vector<F, N>` und Farben |
| `geometry/` | Strahl, Kugel, Dreieck und Box mit ihren Schnitttests (aus dem Labor) |
| `scene/` | `Scene` mit Objekten, Materialien und Lichtquellen sowie den Suchen `nearest_hit` und `any_hit`. Objekte sind Unterklassen von `SceneObject`, die Schnittverfahren `MoellerTrumboreTriangle` und `BadouelTriangle` überschreiben `intersects`. |
| `render/` | `Camera` (Sehstrahl je Bildstelle), `Shader` (Beleuchtung nach Whitted, Rekursion) und `Renderer` (Schleife über die Pixel) |
| `io/` | Wavefront-Import und BMP-Ausgabe |
| `scenes/` | Testszene `teapot_n_glass` |
| `bericht/` | Projektbericht, Messskript und Klassendiagramm (`bericht/uml/klassen.puml`) |
