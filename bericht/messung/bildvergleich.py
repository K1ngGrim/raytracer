# Vergleicht die Bilder beider Schnittverfahren je Aufloesung und schreibt bildvergleich.csv.
# Aufruf aus bericht/messung:  python3 -I bildvergleich.py
import struct, sys, os

def load(path):
    b = open(path, 'rb').read()
    off = struct.unpack_from('<I', b, 10)[0]
    w, h = struct.unpack_from('<ii', b, 18)
    row = (w * 3 + 3) & ~3
    px = {}
    for y in range(h):
        for x in range(w):
            i = off + (h - 1 - y) * row + x * 3
            px[(x, y)] = (b[i + 2], b[i + 1], b[i])
    return w, h, px

here = os.path.dirname(os.path.abspath(__file__))
lines = ["breite,hoehe,pixel,klein"]
for w, h in ((160, 120), (320, 240), (640, 480)):
    _, _, a = load(os.path.join(here, f"mt_{w}x{h}_s1.bmp"))
    _, _, b = load(os.path.join(here, f"badouel_{w}x{h}_s1.bmp"))
    diff = [(k, max(abs(p - q) for p, q in zip(a[k], b[k]))) for k in a if a[k] != b[k]]
    small = sum(1 for _, d in diff if d <= 1)
    lines.append(f"{w},{h},{len(diff)},{small}")
    big = [(k, d) for k, d in diff if d > 1]
    print(f"{w}x{h}: {len(diff)} Pixel, {small} mit max. 1 Stufe; groessere Abweichungen:", file=sys.stderr)
    for k, d in sorted(big)[:40]:
        print(f"   {k} max {d} mt={a[k]} badouel={b[k]}", file=sys.stderr)
open(os.path.join(here, "bildvergleich.csv"), "w").write("\n".join(lines) + "\n")
