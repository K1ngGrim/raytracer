#import "../daten.typ": *
#import "@preview/zero:0.4.0": ztable

= Aufgabe 1: Brute-Force-Raytracer <sec:aufgabe1>

In der ersten Aufgabe entsteht ein Whitted-Raytracer ohne Beschleunigungsstruktur, der jeden Strahl gegen alle Dreiecke der Szene testet. Er dient als Referenz für die folgenden Aufgaben. Im Mittelpunkt stehen die beiden Schnitttests zwischen Strahl und Dreieck nach Badouel und nach Möller und Trumbore, die über die Option `--badouel` gegeneinander ausgetauscht werden.

== Programmentwurf <sec:entwurf>

Grundlage ist die Quelltextbasis aus dem Computergrafiklabor. Das Programm ist in C++20 geschrieben, wird mit CMake gebaut und braucht außer GoogleTest keine Bibliotheken. Der Quelltext liegt in fünf Verzeichnissen. `math/` enthält Vektoren und Farben und `geometry/` Strahl, Kugel und Dreieck aus dem Labor. `scene/` fasst Objekte, Materialien und Lichtquellen in der Klasse `Scene` zusammen, `render/` berechnet daraus das Bild und `io/` enthält den Wavefront-Import und die BMP-Ausgabe. @abb:klassen zeigt die Klassen der drei oberen Verzeichnisse. Jede Klasse hat dabei eine Aufgabe. Der `Renderer` läuft über alle Pixel, die `Camera` erzeugt den Sehstrahl zu einer Stelle im Bild und der `Shader` berechnet die Farbe entlang eines Strahls. In einer früheren Fassung hieß die Pixelschleife `Window`, obwohl das Programm kein Fenster öffnet, und die Kamera berechnete auch die Beleuchtung.

#figure(
  image("../uml/klassen.svg", width: 84%),
  caption: [Klassendiagramm des Raytracers (`uml/klassen.puml`)],
) <abb:klassen>

Ein Objekt der Szene ist eine Unterklasse von `SceneObject`. Die abstrakte Methode `intersects` liefert den Schnitt mit einem Strahl, `shading_normal` die Normale für die Beleuchtung und `ray_starts_inside` die Richtung der Brechung. Die beiden Schnittverfahren sind eigene Unterklassen des Dreiecks und überschreiben nur `intersects`. Welches Verfahren gilt, entscheidet der Import einmal beim Anlegen der Dreiecke. Die Suchen der Szene rufen danach nur noch `intersects` auf und kennen das Verfahren nicht. In einer früheren Fassung fragten beide Suchen bei jedem einzelnen Test ab, welches Verfahren eingestellt ist.

Die Szene hält die Objekte als Zeiger in einem `std::vector`. Ein Dreieck nach Möller-Trumbore belegt 128 Byte, davon 72 Byte für drei Eckpunkte und drei Eckpunktnormalen, 48 Byte für das Material und 8 Byte für den Zeiger auf die Tabelle der virtuellen Methoden. Das Dreieck nach Badouel speichert zusätzlich seine Ebene und belegt 152 Byte. Die rund 2,5 MB der Testszene werden bei der Brute-Force-Suche für jeden Strahl einmal vollständig durchlaufen.

Die Suche `nearest_hit` testet jedes Objekt und merkt sich den nächsten Treffer. Erst für diesen wird die Normale aus den Eckpunktnormalen interpoliert. Die Suche `any_hit` dient dem Schattentest und bricht beim ersten Treffer zwischen Punkt und Licht ab. Der Aufwand beider Suchen wächst damit mit dem Produkt aus der Zahl der Strahlen und der Zahl der Dreiecke. Der Renderer erzeugt je Pixel einen oder mehrere Sehstrahlen an zufälligen Stellen im Pixel und mittelt ihre Farben. Folgestrahlen werden bis zur Tiefe 5 verfolgt, ein Strahl ohne Treffer liefert einen hellblauen Himmel.

Der Wavefront-Import liest Eckpunkte, Normalen und Flächen in einem Durchlauf, zerlegt Polygone in Dreiecke und übernimmt die Eckpunktnormalen, sodass die Teekanne glatt erscheint. Kamera und Licht bestimmt das Programm aus dem Quader um alle Dreiecke. Die Kamera steht bei (0; 8,47; −12,02) auf der Seite mit negativem _z_. Sie blickt aus 30° Höhe mit 40° Öffnungswinkel auf die Mitte der Szene. Die Punktlichtquelle steht bei (1,36; 6,96; −2,71) über der Szene. Die 118 Unit-Tests mit GoogleTest liegen neben dem getesteten Code. Einer schickt 2000 Zufallsstrahlen in eine Szene aus 200 zufälligen Dreiecken und verlangt von beiden Schnittverfahren denselben Treffer im selben Abstand.

== Schnittverfahren <sec:schnitt>

Beide Verfahren bestimmen den Abstand des Treffers und zwei baryzentrische Koordinaten, mit denen die Normale interpoliert wird. Beide schneiden auch Rückseiten, da die Glastöpfe von innen getroffen werden. Das Verfahren nach Badouel @badouel1990 schneidet zuerst den Strahl mit der Ebene des Dreiecks. Normale und Abstand der Ebene hängen nur vom Dreieck ab und werden beim Anlegen einmal berechnet. Aus dem Abstand entlang des Strahls folgt der Schnittpunkt. Danach projiziert Badouel Punkt und Dreieck auf die Koordinatenebene, in der das Dreieck am größten erscheint. Dazu entfällt die Koordinate, in der die Normale ihre größte Komponente hat. In der Ebene wird der Punkt über zwei Kanten des Dreiecks ausgedrückt und das Gleichungssystem mit zwei Unbekannten gelöst (@lst:badouel). Steht die erste Kante senkrecht auf der ersten Achse, gilt dafür ein eigener Zweig. Nach der ersten Koordinate kann der Test abbrechen. Die frühere Fassung verwendete hier den Test aus dem Labor, der im Raum mit drei Kreuzprodukten prüft, ob der Punkt für jede Kante auf der Innenseite liegt. Das entspricht nicht dem Verfahren nach Badouel und wurde ersetzt.

#figure(
  caption: [Schnitttest nach Badouel (`scene/scene_object.cc`, gekürzt)],
  text(size: 8pt)[
```cpp
float t = -(plane_d + normal * ray.origin) / (normal * ray.direction);
if (t < 0.f) return false;
Vector3df p = ray.origin + t * ray.direction;
float u0 = p[axis_u] - a[axis_u], v0 = p[axis_v] - a[axis_v];   // Punkt, b und c relativ zu a
float u1 = b[axis_u] - a[axis_u], v1 = b[axis_v] - a[axis_v];
float u2 = c[axis_u] - a[axis_u], v2 = c[axis_v] - a[axis_v];
if (u1 == 0.f) { beta = u0 / u2;                                  // b - a senkrecht zur ersten Achse
    if (beta < 0.f || beta > 1.f) return false;  alpha = (v0 - beta * v2) / v1; }
else { beta = (v0 * u1 - u0 * v1) / (v2 * u1 - u2 * v1);
    if (beta < 0.f || beta > 1.f) return false;  alpha = (u0 - beta * u2) / u1; }
if (alpha < 0.f || alpha + beta > 1.f) return false;
```
  ],
) <lst:badouel>

Das Verfahren nach Möller und Trumbore @moeller1997 beschreibt den Trefferpunkt direkt über zwei Kanten des Dreiecks und löst das entstehende Gleichungssystem mit der Cramerschen Regel. Ebene und Schnittpunkt werden nicht gebraucht und der Test kann schon nach der ersten und nach der zweiten baryzentrischen Koordinate abbrechen. Da bei der Brute-Force-Suche fast jeder Test sein Dreieck verfehlt, zählt vor allem der Weg bis zur ersten Abbruchprüfung. Möller-Trumbore braucht bis dahin 13 Multiplikationen und eine Division. Badouel braucht trotz der vorab berechneten Ebene ebenfalls 13 Multiplikationen, aber zwei Divisionen, von denen die zweite auf das Ergebnis der ersten wartet.

== Beleuchtung und Materialien <sec:beleuchtung>

Die Farbe in einem Trefferpunkt setzt sich nach Whitted @whitted1980 aus dem direkten Licht und zwei Folgestrahlen zusammen. Alle Gewichte stammen aus der Materialdatei. Die ambiente Farbe gilt überall. Für jede Lichtquelle, die der Schattenstrahl erreicht, kommen der diffuse Anteil nach Lambert und ein Glanzlicht nach Phong @phong1975 hinzu. Die spiegelnde Farbe bestimmt die Stärke des Glanzlichts und der Exponent seine Schärfe. Dieselbe spiegelnde Farbe gewichtet auch den gespiegelten Strahl. Die Deckkraft gibt laut MTL-Spezifikation an, wie stark die Oberfläche das Dahinterliegende überdeckt. Ambiente Farbe, diffuser Anteil und Glanzlicht zählen deshalb nur mit der Deckkraft, den Rest liefert der gebrochene Strahl. In der Testszene spiegeln damit der Porzellantopf und der Glastopf. Teekanne und Tontopf sowie die Platte aus Aluminium haben keine spiegelnde Farbe und bleiben matt. In einer früheren Fassung spiegelte nur ein Material mit dem Beleuchtungsmodell 3 oder 5., sodass die Testszene keine Spiegelung enthielt. Der Schattenstrahl startet leicht entlang der Normale versetzt. Seine Richtung ist der unnormierte Vektor zum Licht, sodass jeder Treffer vor dem Licht Schatten wirft. Zeigt die Fläche vom Licht weg, entfällt er ganz. Der gebrochene Strahl folgt dem Brechungsgesetz von Snellius. Ob er in das Glas eintritt oder es verlässt, entscheidet die Orientierung des Dreiecks. Bei Totalreflexion geht sein Gewicht an den gespiegelten Strahl. Glas wirft wie jedes andere Objekt einen vollen Schatten.

== Bildergebnisse <sec:bilder>

#figure(
  grid(
    columns: 3,
    gutter: 6pt,
    image("../bilder/mt_640x480_s4.png", width: 100%),
    image("../bilder/ausschnitt_s1.png", width: 100%, scaling: "pixelated"),
    image("../bilder/ausschnitt_s4.png", width: 100%, scaling: "pixelated"),
    [(a) 640 × 480, 4 Sehstrahlen], [(b) Ausschnitt, 1 Sehstrahl], [(c) Ausschnitt, 4 Sehstrahlen],
  ),
  caption: [Testszene mit Möller-Trumbore und vergrößerter Ausschnitt um den Glastopf],
) <abb:bilder>

#let vergleich = csv("../messung/bildvergleich.csv", row-type: dictionary)
#let vgl(w) = vergleich.find(r => r.breite == str(w))

@abb:bilder zeigt die Testszene mit der festgelegten Kamera. Zu erkennen sind die harten Schatten auf der Platte, die Spiegelung im Porzellantopf und die Brechung im Glastopf, durch den Platte und Rand verzerrt erscheinen. Auf Porzellan und Glas liegt zudem ein Glanzlicht. Im vergrößerten Ausschnitt entstehen mit einem Sehstrahl je Pixel an den Rändern und Schattengrenzen Treppenstufen. Mit vier Strahlen an zufälligen Stellen im Pixel werden die Übergänge weicher. Die Bilder beider Schnittverfahren unterscheiden sich bei 160 × 120 Pixeln in #vgl(160).pixel von #num(str(160 * 120)) und bei 320 × 240 Pixeln in #vgl(320).pixel von #num(str(320 * 240)) Pixeln, jeweils um höchstens eine Stufe. Bei 640 × 480 Pixeln sind es #vgl(640).pixel von #num(str(640 * 480)), davon #vgl(640).klein um höchstens eine Stufe. Ursache sind Rundungsfehler. Ein Pixel am Rand der Platte weicht dabei stark ab. Dort läuft der Sehstrahl genau über die gemeinsame Kante zweier Dreiecke. Möller-Trumbore verfehlt durch Rundung beide Dreiecke und zeigt den Himmel, Badouel trifft eines davon. Keines der beiden Verfahren ist an gemeinsamen Kanten wasserdicht.

== Zeitmessungen <sec:messung>

#let tests(v, w, h, s) = zaehler(v, w, h, s, "tests")
#let je-test(v, w, h, s) = zeit(v, w, h, s) * 1e9 / tests(v, w, h, s)
#let mt-ref = zeit("mt", 320, 240, 1)
#let ba-ref = zeit("badouel", 320, 240, 1)
#let streuung(v) = { let zs = zeiten(v, 320, 240, 1); (calc.max(..zs) - calc.min(..zs)) / calc.min(..zs) * 100 }

==== Vergleich der Schnittverfahren.
@tab:verfahren stellt beide Verfahren in der Referenzkonfiguration gegenüber, also bei 320 × 240 Pixeln, einem Sehstrahl je Pixel und Rekursionstiefe 5.

#figure(
  caption: [Vergleich der Schnittverfahren (320 × 240 Pixel, 1 Sehstrahl je Pixel, 3 Läufe)],
  text(size: 9pt)[
    #ztable(
      columns: 7,
      align: (left, right, right, right, right, right, right),
      format: (none, auto, auto, auto, auto, auto, auto),
      table.header([*Verfahren*], [*Laden [ms]*], [*Min [s]*], [*Median [s]*], [*Mittel [s]*], [*je Pixel [µs]*], [*je Test [ns]*]),
      ..for (name, v) in (("Badouel", "badouel"), ("Möller-Trumbore", "mt")) {
        let zs = zeiten(v, 320, 240, 1)
        (
          [#name],
          fest(laden(v, 320, 240, 1), 0),
          fest(calc.min(..zs), 2),
          fest(median(zs), 2),
          fest(mittel(zs), 2),
          fest(median(zs) * 1e6 / (320 * 240), 0),
          fest(je-test(v, 320, 240, 1), 2),
        )
      },
    )
  ],
) <tab:verfahren>

Möller-Trumbore ist um den Faktor #z(ba-ref / mt-ref, stellen: 2) schneller. Die Läufe streuen um höchstens #z(calc.max(streuung("mt"), streuung("badouel")), stellen: 1) %, der Unterschied ist also kein Messrauschen. Beide Verfahren führen dieselben #num(str(tests("mt", 320, 240, 1))) Schnitttests aus. Ein Test dauert damit #z(je-test("mt", 320, 240, 1), stellen: 2) ns gegenüber #z(je-test("badouel", 320, 240, 1), stellen: 2) ns. Bei gleicher Zahl an Multiplikationen erklärt vor allem die zweite Division den Abstand. Sie wartet auf den Schnittpunkt und dieser auf die erste Division, sodass der Prozessor die Rechnungen kaum überlappen kann. Zudem liest Badouel die Koordinaten über die gespeicherten Achsen einzeln aus dem Speicher und jedes Objekt ist 24 Byte größer. In einer früheren Fassung ohne virtuelle Methoden dauerte ein Test nach Möller-Trumbore 7,87 ns. Der Aufruf über die Tabelle der virtuellen Methoden und die einzeln angelegten Objekte kosten damit rund #z((je-test("mt", 320, 240, 1) / 7.87 - 1) * 100, stellen: 0) %.

#let prim = zaehler("mt", 320, 240, 1, "primaer")
#let folge = zaehler("mt", 320, 240, 1, "folge")
#let schatten = zaehler("mt", 320, 240, 1, "schatten")
#let alle-tests = tests("mt", 320, 240, 1)
#let schatten-tests = alle-tests - (prim + folge) * dreiecke

==== Strahlen und Schnitttests.
@tab:strahlen schlüsselt die Strahlen der Referenzkonfiguration nach ihrer Art auf.

#figure(
  caption: [Strahlen und Schnitttests bei 320 × 240 Pixeln (Möller-Trumbore)],
  text(size: 9pt)[
    #ztable(
      columns: 4,
      align: (left, right, right, right),
      format: (none, auto, auto, auto),
      table.header([*Strahlart*], [*Strahlen*], [*Schnitttests*], [*Tests je Strahl*]),
      [Primärstrahlen], str(prim), str(prim * dreiecke), str(dreiecke),
      [Folgestrahlen (Spiegelung, Brechung)], str(folge), str(folge * dreiecke), str(dreiecke),
      [Schattenstrahlen], str(schatten), str(schatten-tests), fest(schatten-tests / schatten, 0),
      [Summe], str(prim + folge + schatten), str(alle-tests), fest(alle-tests / (prim + folge + schatten), 0),
    )
  ],
) <tab:strahlen>

Primär- und Folgestrahlen prüfen alle #num(str(dreiecke)) Dreiecke. Der Abbruch beim ersten Treffer spart wenig, ein Schattenstrahl prüft im Mittel #z(schatten-tests / schatten / dreiecke * 100, stellen: 0) % der Dreiecke, da die meisten das Licht erreichen.

#let konfig = ((160, 120, 1), (320, 240, 1), (640, 480, 1), (320, 240, 4), (320, 240, 8), (320, 240, 16))
#let faktor(w, h, s) = zeit("mt", w, h, s) / mt-ref
#let erwartet(w, h, s) = w * h * s / (320 * 240)
#let speedups = ((160, 120), (320, 240), (640, 480)).map(((w, h)) => zeit("badouel", w, h, 1) / zeit("mt", w, h, 1))

==== Skalierung mit Auflösung und Sehstrahlen je Pixel.
Da jeder Strahl gleich viele Dreiecke prüft, sollte die Laufzeit proportional zur Zahl der Pixel und zur Zahl der Sehstrahlen je Pixel wachsen. @tab:skalierung vergleicht den gemessenen mit dem erwarteten Faktor.

#figure(
  caption: [Laufzeit von Möller-Trumbore nach Auflösung und Sehstrahlen je Pixel (Median aus 3 Läufen)],
  text(size: 9pt)[
    #ztable(
      columns: 6,
      align: (left, right, right, right, right, right),
      format: (none, auto, auto, auto, auto, auto),
      table.header([*Auflösung*], [*Sehstrahlen je Pixel*], [*Laufzeit [s]*], [*je Sehstrahl [µs]*], [*Faktor gem.*], [*Faktor erw.*]),
      ..for (w, h, s) in konfig {
        let t = zeit("mt", w, h, s)
        ([#w × #h], str(s), fest(t, 2), fest(t * 1e6 / (w * h * s), 1), fest(faktor(w, h, s), 3), fest(erwartet(w, h, s), 2))
      },
    )
  ],
) <tab:skalierung>

Die Laufzeit wächst wie erwartet linear. Keine Messung weicht um mehr als #z(calc.max(..konfig.slice(1).map(((w, h, s)) => calc.abs(faktor(w, h, s) / erwartet(w, h, s) - 1) * 100)), stellen: 1) % vom erwarteten Faktor ab. Badouel wurde zusätzlich bei allen drei Auflösungen gemessen und ist dort um den Faktor #z(calc.min(..speedups), stellen: 2) bis #z(calc.max(..speedups), stellen: 2) langsamer.

==== Folgerung.
Der Aufwand des Brute-Force-Raytracers wächst mit dem Produkt aus der Zahl der Strahlen und der Zahl der Dreiecke. Hochgerechnet auf die voreingestellte Auflösung von 1280 × 720 Pixeln mit 16 Sehstrahlen je Pixel ergibt sich rund das #z(1280 * 720 * 16 / (320 * 240), stellen: 0)-Fache der Referenzmessung, also etwa #z(mt-ref * 1280 * 720 * 16 / (320 * 240) / 60, stellen: 0) Minuten für ein einziges Bild. Wesentlich schneller wird es nur, wenn ein Strahl deutlich weniger Dreiecke prüft. Dafür kann ein k-d-Baum hinter den beiden Suchen der Szene die Brute-Force-Suche ersetzen.
