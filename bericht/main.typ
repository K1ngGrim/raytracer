#import "template/template.typ": *
#import "@preview/zero:0.4.0": set-num, set-group

// Zahlen wie im deutschen Satz: Dezimalkomma und schmale Leerzeichen ab fuenf Stellen
#set-num(decimal-separator: ",", math: false)
#set-group(separator: sym.space.nobreak.narrow, threshold: 5)

#show: project.with(
  type: "Laborbericht",
  title: "Optimierung eines Raytracers",
  subtitle: "",
  authors: (
    (
      name: "Florian Kaiser",
      matriculation_number: "79728",
      email: "kafl1018@h-ka.de",
    ),
  ),
  date: datetime.today().display("[day].[month].[year]"),
  company_logo: none,
  logo_left: "logo_left.png",
  logo_right: "logo_right.png",
  institution: "Hochschule Karlsruhe",
  study_program: "Informatik Master",
  custom_front_page: [],
  examiner: "Prof. Dr. Christian Pape",
  ai_usage_disclosure: false,
  show_outline: false,
)

#include "kapitel/01_einleitung.typ"
#include "kapitel/02_aufgabe1.typ"
// #include "kapitel/03_aufgabe2.typ"
// #include "kapitel/04_aufgabe3.typ"
// #include "kapitel/05_fazit.typ"

// Literatur ohne eigenes Kapitel, damit sie keinen Seitenumbruch erzwingt
#v(1em)
#text(size: 1.2em, weight: "bold")[Literatur]
#text(size: 9pt, bibliography("quellen.bib", title: none))
