// Auswertung der Messreihe aus messung/messungen.csv (eine Zeile je Lauf, erzeugt von messung/messen.sh)
#import "@preview/zero:0.4.0": num

#let roh = csv("messung/messungen.csv", row-type: dictionary)
#let dreiecke = 18032

#let laeufe(verfahren, breite, hoehe, samples) = roh.filter(r => (
  r.verfahren == verfahren and r.breite == str(breite) and r.hoehe == str(hoehe) and r.samples == str(samples)
))

#let median(werte) = {
  let s = werte.sorted()
  let n = s.len()
  if calc.odd(n) { s.at(int((n - 1) / 2)) } else { (s.at(int(n / 2) - 1) + s.at(int(n / 2))) / 2 }
}
#let mittel(werte) = werte.sum() / werte.len()

// Renderzeiten aller Laeufe einer Konfiguration in Sekunden
#let zeiten(v, w, h, s) = laeufe(v, w, h, s).map(r => float(r.rendern_ms) / 1000)
#let zeit(v, w, h, s) = median(zeiten(v, w, h, s))
#let laden(v, w, h, s) = median(laeufe(v, w, h, s).map(r => float(r.laden_ms)))
// Zaehler sind in allen Laeufen einer Konfiguration gleich, es genuegt der erste Lauf
#let zaehler(v, w, h, s, feld) = int(laeufe(v, w, h, s).first().at(feld))

// Zahl mit fester Anzahl Nachkommastellen als Zeichenkette mit Dezimalpunkt (fuer zero)
#let fest(x, stellen) = {
  if stellen == 0 { return str(int(calc.round(x))) }
  let r = calc.round(x, digits: stellen)
  let teile = str(r).split(".")
  let nachkomma = if teile.len() > 1 { teile.at(1) } else { "" }
  teile.at(0) + "." + nachkomma + "0" * (stellen - nachkomma.len())
}

// formatierte Zahl fuer den Fliesstext und fuer Tabellenzellen
#let z(x, stellen: 1) = num(fest(x, stellen))
