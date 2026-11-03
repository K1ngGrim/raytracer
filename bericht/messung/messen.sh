#!/bin/sh
# Zeitmessung fuer den Bericht. Baut den Raytracer im Release-Modus und misst jede
# Konfiguration mehrfach auf der Testszene. Kamera und Licht legt das Programm selbst
# aus der Szene fest, sie sind fuer alle Messungen gleich.
#
# Aufruf aus dem Wurzelverzeichnis des Projekts:  sh bericht/messung/messen.sh [laeufe]
# Ergebnis: bericht/messung/messungen.csv (eine Zeile je Lauf) und die Bilder in bericht/bilder/

set -e
RUNS=${1:-3}
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
OUT="$ROOT/bericht/messung"
BUILD="$OUT/build"
SCENE="$ROOT/scenes/teapot_n_glass/teapot_n_glass.obj"
CSV="$OUT/messungen.csv"

cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build "$BUILD" --target raytracer -j > /dev/null
BIN="$OUT/raytracer.bin"
cp "$BUILD/raytracer" "$BIN"   # eigene Kopie, damit ein neuer Build die laufende Messung nicht stoert

echo "verfahren,breite,hoehe,samples,lauf,laden_ms,rendern_ms,primaer,folge,schatten,tests" > "$CSV"

# misst eine Konfiguration: verfahren breite hoehe samples
measure() {
    method=$1; w=$2; h=$3; s=$4
    flag=""; [ "$method" = "badouel" ] && flag="--badouel"
    img="$OUT/tmp_${method}_${w}x${h}_s${s}.bmp"
    for run in $(seq 1 "$RUNS"); do
        log=$("$BIN" "$SCENE" -w "$w" -h "$h" -s "$s" $flag -o "$img")
        load=$(echo "$log" | sed -n 's/^TIME: load \(.*\) ms$/\1/p')
        render=$(echo "$log" | sed -n 's/^TIME: render \(.*\) ms$/\1/p')
        stats=$(echo "$log" | sed -n 's/^STATS: primary \(.*\), secondary \(.*\), shadow \(.*\), tests \(.*\)$/\1,\2,\3,\4/p')
        echo "$method,$w,$h,$s,$run,$load,$render,$stats" >> "$CSV"
        echo "$method ${w}x${h} s$s Lauf $run: $render ms"
    done
    cp "$img" "$OUT/${method}_${w}x${h}_s${s}.bmp"
    rm -f "$img"
}

# Aufwaermlauf, wird nicht gespeichert
"$BIN" "$SCENE" -w 160 -h 120 -o "$OUT/tmp_warmup.bmp" > /dev/null
rm -f "$OUT/tmp_warmup.bmp"

# Vergleich der Schnittverfahren und Abhaengigkeit von der Aufloesung
for res in "160 120" "320 240" "640 480"; do
    set -- $res
    measure mt "$1" "$2" 1
    measure badouel "$1" "$2" 1
done

# Abhaengigkeit von der Anzahl der Sehstrahlen pro Pixel
for s in 4 8 16; do
    measure mt 320 240 "$s"
done

# Bilder fuer den Bericht (nur einmal, gehoeren nicht zur Zeitmessung)
"$BIN" "$SCENE" -w 640 -h 480 -s 4 -o "$OUT/mt_640x480_s4.bmp" > /dev/null
"$BIN" "$SCENE" -w 160 -h 120 -s 8 -o "$OUT/mt_160x120_s8.bmp" > /dev/null
IMG="$ROOT/bericht/bilder"
for name in mt_160x120_s8 mt_320x240_s1 mt_640x480_s4; do
    sips -s format png "$OUT/$name.bmp" --out "$IMG/$name.png" > /dev/null
done
# Ausschnitt um den Glastopf mit einem und mit vier Sehstrahlen je Pixel
sips -s format png -c 150 200 --cropOffset 290 220 "$OUT/mt_640x480_s1.bmp" --out "$IMG/ausschnitt_s1.png" > /dev/null
sips -s format png -c 150 200 --cropOffset 290 220 "$OUT/mt_640x480_s4.bmp" --out "$IMG/ausschnitt_s4.png" > /dev/null

# Vergleich der Bilder beider Schnittverfahren
python3 -I "$OUT/bildvergleich.py"

echo "fertig: $CSV"
