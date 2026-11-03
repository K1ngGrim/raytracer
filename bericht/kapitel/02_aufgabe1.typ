#import "../daten.typ": *
#import "@preview/zero:0.4.0": ztable

= Aufgabe 1: Brute-Force-Raytracer <sec:aufgabe1>

In der ersten Aufgabe entsteht ein Whitted-Raytracer ohne Beschleunigungsstruktur, der jeden Strahl gegen alle Dreiecke der Szene testet. Er dient als Referenz für die folgenden Aufgaben. Im Mittelpunkt stehen die beiden Schnitttests zwischen Strahl und Dreieck nach Badouel und nach Möller und Trumbore, die über die Option `--badouel` gegeneinander ausgetauscht werden.

== Programmentwurf <sec:entwurf>

Grundlage ist die Quelltextbasis aus dem Computergrafiklabor. Das Programm ist in C++20 geschrieben, wird mit CMake gebaut und braucht außer GoogleTest keine Bibliotheken. Die Klassen liegen in fünf aufeinander aufbauenden Verzeichnissen. `math/` enthält die Vektorklasse und `geometry/` Strahl, Kugel und Dreieck mit ihren Schnitttests. Beide sind Templates über Zahlentyp und Dimension, verwendet werden dreidimensionale Vektoren aus `float`. `world/` fasst Objekte, Materialien und Lichtquellen in der Klasse `World` zusammen. In `view/` erzeugen Kamera, Bildebene und Fenster aus der Welt das Bild und `utility/` enthält den Wavefront-Import und die BMP-Ausgabe. Alles, was von der Art der Suche abhängt, liegt in `World`. Die Kamera ruft nur deren zwei Suchen auf und bleibt deshalb unverändert, wenn in den folgenden Aufgaben eine Beschleunigungsstruktur hinzukommt.

Ein Vektor belegt 12 Byte, ein Dreieck aus drei Eckpunkten und drei Eckpunktnormalen 72 Byte. Die Szene ist ein `std::vector`, in dem die Objekte als Werte hintereinander liegen. Ein Objekt enthält wie im Labor sowohl eine Kugel als auch ein Dreieck und ein Feld, das angibt, welches davon gilt. Das kommt ohne virtuelle Methoden aus, kostet aber 112 Byte je Objekt, obwohl der Schnitttest nur die 36 Byte der Eckpunkte braucht. Die rund 2 MB der Testszene werden bei der Brute-Force-Suche für jeden Strahl einmal vollständig durchlaufen.

Die Suche `find_nearest_object` testet jedes Objekt und merkt sich den nächsten Treffer. Erst für diesen wird die Normale aus den Eckpunktnormalen interpoliert. Die Suche `ray_intersects_any` dient dem Schattentest und bricht beim ersten Treffer zwischen Punkt und Licht ab. Der Aufwand beider Suchen wächst damit mit dem Produkt aus der Zahl der Strahlen und der Zahl der Dreiecke. Für jedes Pixel erzeugt das Fenster einen oder mehrere Sehstrahlen an zufälligen Stellen im Pixel und mittelt ihre Farben. Die Kamera sucht für jeden Strahl den nächsten Treffer und verzweigt nach dem Material. Spiegel und Glas erzeugen rekursiv Folgestrahlen bis zur Tiefe 5, ein Strahl ohne Treffer liefert einen hellblauen Himmel. Das fertige Bild wird als 24-Bit-BMP geschrieben.

Der Wavefront-Import liest Eckpunkte, Normalen und Flächen in einem Durchlauf, zerlegt Polygone in Dreiecke und übernimmt die Eckpunktnormalen, sodass die Teekanne glatt erscheint. Ein durchsichtiges Material wird zu Glas mit dem Brechungsindex aus der Materialdatei. In der Testszene ist damit der Glastopf aus Glas und alles andere diffus. Kamera und Licht bestimmt das Programm aus dem Quader um alle Dreiecke. Die Kamera steht bei (0; 8,47; −12,02) auf der Seite mit negativem _z_, da von der anderen Seite die Teekanne zwei der Töpfe verdeckt. Sie blickt aus 30° Höhe mit 40° Öffnungswinkel auf die Mitte der Szene. Die Punktlichtquelle steht bei (1,36; 6,96; −2,71) über der Szene.

Die 109 Unit-Tests mit GoogleTest liegen neben dem getesteten Code. Einer schickt 2000 Zufallsstrahlen in eine Szene aus 200 zufälligen Dreiecken und verlangt von beiden Schnittverfahren denselben Treffer im selben Abstand. Ein anderer zählt die Schnitttests und prüft so den Abbruch des Schattentests.

== Schnittverfahren <sec:schnitt>

Beide Verfahren bestimmen für einen Strahl und ein Dreieck den Abstand des Treffers und zwei baryzentrische Koordinaten, mit denen die Normale interpoliert wird. Beide schneiden auch Rückseiten, da die Glastöpfe von innen getroffen werden.

Das Verfahren nach Badouel @badouel1990 schneidet zuerst den Strahl mit der Ebene des Dreiecks und berechnet daraus den Schnittpunkt. Danach wird geprüft, ob dieser Punkt im Dreieck liegt. Badouel projiziert dazu Punkt und Dreieck auf eine Koordinatenebene und rechnet dort in zwei Dimensionen weiter. Die Umsetzung aus dem Labor prüft stattdessen im Raum mit drei Kreuzprodukten, ob der Punkt für jede Kante auf der Innenseite liegt. Die baryzentrischen Koordinaten ergeben sich dort aus den Flächen der Teildreiecke.

Das Verfahren nach Möller und Trumbore @moeller1997 beschreibt den Trefferpunkt direkt über zwei Kanten des Dreiecks und löst das entstehende Gleichungssystem mit der Cramerschen Regel. Ebene und Schnittpunkt werden nicht gebraucht und das Verfahren kann schon nach der ersten und nach der zweiten baryzentrischen Koordinate abbrechen (@lst:mt). Da bei der Brute-Force-Suche fast jeder Test sein Dreieck verfehlt, spart das den größten Teil der Arbeit. Bis zur ersten Abbruchprüfung braucht Möller-Trumbore 13 Multiplikationen und eine Division. Die Variante nach Badouel braucht bis dahin 30 Multiplikationen, eine Division und eine Wurzel für die Länge der Normale.

#figure(
  caption: [Schnitttest nach Möller und Trumbore (`geometry/geometry.tcc`, gekürzt)],
  text(size: 8pt)[
```cpp
Vector<FLOAT, N> edge1 = b - a, edge2 = c - a;
Vector<FLOAT, N> p_vec = ray.direction.cross_product(edge2);
FLOAT det = edge1 * p_vec;
if ( fabs(det) < EPSILON ) return false;               // Strahl parallel zum Dreieck
FLOAT inv_det = 1.0 / det;
Vector<FLOAT, N> t_vec = ray.origin - a;
FLOAT bary_b = (t_vec * p_vec) * inv_det;               // Gewicht von b
if ( bary_b < 0.0 || bary_b > 1.0 ) return false;
Vector<FLOAT, N> q_vec = t_vec.cross_product(edge1);
FLOAT bary_c = (ray.direction * q_vec) * inv_det;       // Gewicht von c
if ( bary_c < 0.0 || bary_b + bary_c > 1.0 ) return false;
FLOAT t = (edge2 * q_vec) * inv_det;
if ( t < 0.0 ) return false;
```
  ],
) <lst:mt>

== Beleuchtung und Materialien <sec:beleuchtung>

Diffuse Flächen werden nach Lambert beleuchtet. Zum Kosinus zwischen Normale und Richtung zum Licht kommt ein ambienter Anteil von 0,1, der überall gilt. In einer früheren Fassung erhielten vom Licht abgewandte Flächen keinen ambienten Anteil. An der Teekanne entstand dadurch ein treppenförmiger Streifen an der Schattengrenze. Der Schattenstrahl startet leicht entlang der Normale versetzt. Seine Richtung ist der unnormierte Vektor zum Licht, sodass jeder Treffer vor dem Licht Schatten wirft. Zeigt die Fläche vom Licht weg, entfällt er ganz. Glas erzeugt einen gebrochenen Strahl nach dem Brechungsgesetz von Snellius und einen reflektierten Strahl. Ihre Farben werden nach der Näherung von Schlick @schlick1994 für den Fresnel-Faktor gemischt. Ob ein Strahl in das Glas eintritt oder es verlässt, entscheidet die Orientierung des Dreiecks. Ist keine Brechung möglich, wird der Strahl total reflektiert. Glas wirft wie jedes andere Objekt einen vollen Schatten.

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

@abb:bilder zeigt die Testszene mit der festgelegten Kamera. Zu erkennen sind die harten Schatten auf der Platte und die Brechung im Glastopf, durch den Platte und Rand verzerrt erscheinen. Im vergrößerten Ausschnitt entstehen mit einem Sehstrahl je Pixel an den Rändern und Schattengrenzen Treppenstufen. Mit vier Strahlen an zufälligen Stellen im Pixel werden die Übergänge weicher. Die Bilder beider Schnittverfahren sind bei 160 × 120 Pixeln gleich. Bei 320 × 240 Pixeln weichen #vgl(320).pixel von #num(str(320 * 240)) Pixeln ab und bei 640 × 480 Pixeln #vgl(640).pixel von #num(str(640 * 480)), jeweils um höchstens eine Stufe in einem Farbkanal. Ursache sind Rundungsfehler bei der Interpolation der Normale.

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

Möller-Trumbore ist um den Faktor #z(ba-ref / mt-ref, stellen: 2) schneller. Die Läufe streuen um höchstens #z(calc.max(streuung("mt"), streuung("badouel")), stellen: 1) %, der Unterschied ist also kein Messrauschen. Beide Verfahren führen dieselben #num(str(tests("mt", 320, 240, 1))) Schnitttests aus. Ein Test dauert damit #z(je-test("mt", 320, 240, 1), stellen: 2) ns gegenüber #z(je-test("badouel", 320, 240, 1), stellen: 2) ns, bei 3,2 GHz sind das etwa #z(je-test("mt", 320, 240, 1) * 3.2, stellen: 0) und #z(je-test("badouel", 320, 240, 1) * 3.2, stellen: 0) Takte. Der Abstand ist größer, als die Zahl der Multiplikationen erwarten lässt. Hinzu kommen bei Badouel die Wurzel und die Abhängigkeiten zwischen den Schritten. Der Schnittpunkt hängt vom Abstand ab und dieser von der Division, sodass der Prozessor die Rechnungen kaum überlappen kann. Spielraum gibt es bei beiden Verfahren. Kanten und Normale werden bei jedem Test neu berechnet und die Schnitttests liegen in einer eigenen Übersetzungseinheit, sodass sie ohne Link-Time-Optimierung nicht eingebettet werden.

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

Primär- und Folgestrahlen prüfen alle #num(str(dreiecke)) Dreiecke. Schattenstrahlen gibt es weniger als Primärstrahlen, da manche Primärstrahlen nur den Himmel treffen und vom Licht abgewandte Flächen keinen brauchen. Der Abbruch beim ersten Treffer spart wenig. Ein Schattenstrahl prüft im Mittel #z(schatten-tests / schatten / dreiecke * 100, stellen: 0) % der Dreiecke, da die meisten das Licht erreichen.

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

Die Laufzeit wächst wie erwartet linear. Keine Messung weicht um mehr als #z(calc.max(..konfig.slice(1).map(((w, h, s)) => calc.abs(faktor(w, h, s) / erwartet(w, h, s) - 1) * 100)), stellen: 1) % vom erwarteten Faktor ab. Da alle Auflösungen denselben Bildausschnitt zeigen, bleibt die mittlere Zahl der Strahlen je Pixel gleich. Zufallszahlen und das Mitteln der Farben fallen gegenüber den rund #z(alle-tests / (prim + folge + schatten), stellen: 0) Schnitttests je Strahl nicht ins Gewicht. Badouel wurde zusätzlich bei allen drei Auflösungen gemessen und ist dort um den Faktor #z(calc.min(..speedups), stellen: 2) bis #z(calc.max(..speedups), stellen: 2) langsamer, da der Unterschied im einzelnen Schnitttest entsteht.

==== Folgerung.
Der Aufwand des Brute-Force-Raytracers wächst mit dem Produkt aus der Zahl der Strahlen und der Zahl der Dreiecke. Hochgerechnet auf die voreingestellte Auflösung von 1280 × 720 Pixeln mit 16 Sehstrahlen je Pixel ergibt sich rund das #z(1280 * 720 * 16 / (320 * 240), stellen: 0)-Fache der Referenzmessung, also etwa #z(mt-ref * 1280 * 720 * 16 / (320 * 240) / 60, stellen: 0) Minuten für ein einziges Bild. Die Wahl des Schnittverfahrens bringt nur einen festen Faktor. Eine wesentliche Beschleunigung ist nur zu erreichen, wenn ein Strahl deutlich weniger Dreiecke prüfen muss. Hinter den beiden Suchen der Welt kann dafür ein k-d-Baum die Brute-Force-Suche ersetzen.
