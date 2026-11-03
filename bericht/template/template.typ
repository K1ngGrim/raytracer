#import "@preview/biceps:0.0.1": *

#let buildMainHeader(mainHeadingContent) = {
  [
    #align(center, smallcaps(mainHeadingContent))
    #line(length: 100%)
  ]
}

#let buildSecondaryHeader(mainHeadingContent, secondaryHeadingContent) = {
  [
    #smallcaps(mainHeadingContent) #h(1fr) #emph(secondaryHeadingContent)
    #line(length: 100%)
  ]
}

// To know if the secondary heading appears after the main heading
#let isAfter(secondaryHeading, mainHeading) = {
  let secHeadPos = secondaryHeading.location().position()
  let mainHeadPos = mainHeading.location().position()
  if (secHeadPos.at("page") > mainHeadPos.at("page")) {
    return true
  }
  if (secHeadPos.at("page") == mainHeadPos.at("page")) {
    return secHeadPos.at("y") > mainHeadPos.at("y")
  }
  return false
}

// Kopfzeile mit aktuellem Kapitel und Unterkapitel
#let getHeader() = {
  context {
    // Finde das letzte Hauptkapitel, das vor oder auf der aktuellen Seite beginnt
    let lastMainHeading = query(selector(heading).before(here())).filter(headIt => headIt.level == 1).at(-1, default: none)

    // Prüfe, ob die aktuelle Seite die erste Seite dieses Hauptkapitels ist
    let isFirstPageOfMainHeading = lastMainHeading != none and lastMainHeading.location().page() >= here().page()

    //Prüfe, ob die aktuelle Seite mit einer neuen Hauptüberschrift beginnt
    let currentMainHeading = query(selector(heading).after(here())).find(headIt => (
      headIt.level == 1 and headIt.location().page() <= here().page()
    ))
    if (currentMainHeading != none) {
      return buildMainHeader(currentMainHeading.body)
    }
    // Wenn dies die erste Seite eines Hauptkapitels ist und keine neue Hauptüberschrift auf der aktuellen Seite beginnt, zeige nur das Hauptkapitel an
    if (isFirstPageOfMainHeading) {
      return buildMainHeader(lastMainHeading.body)
    } /*else if (isFirstPageOfMainHeading and currentMainHeading != none) {
        //return buildMainHeader(currentMainHeading.body)
      }*/

    // if (currentMainHeading != none) {
    //  return buildMainHeader(currentMainHeading.body)
    // }

    // Finde das erste Unterkapitel auf der aktuellen Seite
    let currentSecondaryHeading = query(selector(heading).after(here())).find(headIt => (
      headIt.level == 2 and headIt.location().page() == here().page()
    ))

    // Finde das letzte Unterkapitel, das vor der aktuellen Seite beginnt
    let lastSecondaryHeadingArray = query(selector(heading).before(here())).filter(headIt => headIt.level == 2 or headIt.level == 1)
    let lastSecondaryHeading = if (lastSecondaryHeadingArray.len() != 0) {
      if (lastSecondaryHeadingArray.last().level != 1) {
        lastSecondaryHeadingArray.last()
      }else {
        none
      }
    } else {
      none
    }

    // Wenn ein Unterkapitel auf der aktuellen Seite existiert, nutze dieses

    if (currentSecondaryHeading != none) {
      return buildSecondaryHeader(lastMainHeading.body, currentSecondaryHeading.body)
    }

    // Wenn ein Unterkapitel vor der aktuellen Seite existiert und es nach dem Hauptkapitel liegt
    if (lastSecondaryHeading != none and isAfter(lastSecondaryHeading, lastMainHeading)) {
      return buildSecondaryHeader(lastMainHeading.body, lastSecondaryHeading.body)
    }

    // Ansonsten zeige nur das letzte Hauptkapitel an
    return buildMainHeader(lastMainHeading.body)
  }
}

#let prompt(question, answer) = {
  block(width: 75%, fill: luma(240), inset: 1em, radius: 0.5em)[
    #align(left)[
      #text(size: 8pt)[
        *Prompt:* \
        #question
      
        #if answer != none {
          [
            \
            *LLM Antwort:* \
            #answer
          ]
          
        }
        
      ]
    ]
  ]
}

#let pg(title, content) = {
  v(1em)
  par(
    [#strong(title)#label(title) #content],
    first-line-indent: 0pt
  )
}


#let project(
  type: "",
  title: "",
  subtitle: "",
  module: none,
  study_program: "",
  institution: "",
  date: none,
  examiner: none,
  abstract: [],
  zusammenfassung: [],
  restriction_notice: [],
  statutory_declaration: [],
  acknowledgments: [],
  authors: (),
  logo_left: none,
  logo_right: none,
  custom_front_page: "",
  company_logo: "",
  ai_usage_disclosure: false,
  // Verzeichnisse, die hinter dem Inhaltsverzeichnis und damit noch im
  // roemisch nummerierten Vorspann stehen sollen, etwa das Abbildungs- und
  // das Abkuerzungsverzeichnis.
  front_matter: none,
  // false fuer kurze Berichte ohne Inhaltsverzeichnis
  show_outline: true,
  body,
) = {
  // Set the document's basic properties.
  set document(author: authors.map(a => a.name), title: title)
  set text(font: "New Computer Modern", lang: "de", region: "de", size: 11pt)
  set par(justify: true, linebreaks: "optimized")
  set par(leading: 0.5em, spacing: 0.75em, first-line-indent: 1.8em)

  // The function is passed a variable number of arguments (e.g. 1, 1, 2).
  // The `..nums` is an argument sink, which collects all excess arguments
  // in `nums`.
  set heading(
    numbering: (..nums) => {
      // We want the positional arguments from `nums`.
      // `numbers` is now an array of ints
      // e.g. a level one heading `= ...`  could be (1,)
      // e.g. a level two heading `== ...` could be (2, 1)
      // e.g. a level three heading `=== ...` could be (1, 0, 2)
      let numbers = nums.pos()
      if numbers.len() <= 4 {
        numbering("1.1", ..numbers)
      }
    },
  )

  show heading: it => [
    #if it.level == 1 [
      #colbreak(weak: true)
    ]#pad(bottom: 0.5em)[#it]]
  // Ein Verweis auf eine Überschrift der obersten Ebene ist ein Kapitel, kein
  // Abschnitt. Ohne diese Zuordnung setzt Typst durchgehend "Abschnitt".
  show heading.where(level: 1): set heading(supplement: [Kapitel])
  show heading.where(level: 4): set heading(outlined: false, supplement: [Absatz])
  show heading.where(level: 4): it => {
    parbreak()
    // Kein angehaengter Punkt: die Ueberschriften dieser Ebene sind Fragen und
    // tragen ihr eigenes Satzzeichen, sonst entstuende "vollstaendig?.".
    text(weight: "bold")[#it.body]
    h(0.5em)
  }
  set math.equation(numbering: "(1)", supplement: [Formel])
  show math.equation: set text(weight: 400)
  show raw: set text(font: "New Computer Modern Mono")
  show link: set text(fill: blue.darken(55%))
  show figure: set block(spacing: 1.3em + 3pt)
  // Narrow table cells in justified text open large gaps, and coloured
  // abbreviation links inside figures and tables look restless.
  show table: set par(justify: false)
  // Hyphenation follows justification by default, so it has to be switched
  // on again, or long words overflow narrow cells.
  show table: set text(hyphenate: true)
  show figure: it => {
    show link: set text(fill: black)
    it
  }
  show figure.where(kind: table): set figure.caption(position: top)
  show figure.where(kind: table): set figure(numbering: "I")

  show raw.where(block: false): box.with(
    fill: luma(240),
    inset: (x: 3pt, y: 0pt),
    outset: (y: 3pt),
    radius: 2pt,
  )

  if custom_front_page != [] {
    image(custom_front_page, width: 100%, height: 110%)
  } else {
    v(2em)
    // Logo
    if logo_left != none and logo_right != none {
      grid(
        columns: (1fr, 1fr),
        grid.cell(align(left, image(logo_left, width: 75%))),
        grid.cell(align(right, image(logo_right, width: 40%))),
      )
    } else if logo_right != none {
      align(left, image(logo_left, width: 10%))
    } else if logo_left != none {
      align(right, image(logo_right, width: 10%))
    } else {
      v(0.75fr)
    }

    // Title page.
    v(4.5em)
    align(center)[
      #text(1.1em, weight: 100, type)
    ]
    v(0.5em)
    align(center)[
      #text(1.6em, weight: 700, title)
    ]
    if subtitle != "" {
      v(1em)
      align(center)[
        #text(1.3em, weight: 500, subtitle)
      ]
    }
    v(2em)
    align(center)[
      im Studiengang \
      #study_program
    ]

    v(2em)
    align(center)[
      vorgelegt von
    ]
    // Author information.
    pad(
      top: 0.7em,
      align(
        (center),
        grid(columns: 2, gutter: 1em, ..authors.map(author => align(center)[
            *#author.name* \
            #if "matriculation_number" in author [
              Matrikelnummer:
              #author.matriculation_number \
            ]
            #if "email" in author [
              #link("mailto:" + author.email) \
            ]
            #if "affiliation" in author [
              #author.affiliation \
            ]
            #if "postal" in author [
              #author.postal \
            ]
            #if "phone" in author [
              #author.phone
            ]
          ])),
      ),
    )

    v(2em)
    align(center)[
      an der #institution
    ]

    if date != none {
      align(center)[
        am #date
      ]
    }

    v(4em)
    line(length: 100%)
    if examiner != none {
      par[
        *Prüfer*: #examiner
      ]
    }

    if module != none {
      par[
        *Veranstaltung*: #module
      ]
    }
    pagebreak()
  }


  set page(numbering: "I", number-align: center)

  if statutory_declaration != [] [
    #counter(page).update(1)
    #align(center)[
      #heading(outlined: false, numbering: none, text(0.85em, "ERKLÄRUNG"))
    ]
    #statutory_declaration \ \ \ \
    Karlsruhe, #date
    #line(start: (0%, 10%), length: 30%)
    #authors.at(0).name

  ]

  // ai_usage_disclosure: false (keine Erklaerung) oder der Text der Erklaerung als Inhalt
  if ai_usage_disclosure != false [
    #pagebreak(weak: true)
    #align(center)[
      #heading(outlined: false, numbering: none, text(0.85em, "ERKLÄRUNG ZUR NUTZUNG VON KI-WERKZEUGEN"))
    ]
    #text(11pt)[
      #par(justify: true)[#ai_usage_disclosure]
    ]
  ]

  if restriction_notice != [] [
    #pagebreak()
    #align(center)[
      #heading(outlined: false, numbering: none, text(0.85em, "SPERRVERMERK"))
    ]
    #if company_logo != none [
      #figure(image(company_logo, width: 70%), outlined: false, numbering: none)
    ]
    
    #restriction_notice
    #v(1.618fr)
    //#counter(page).update(2)
    #pagebreak()
  ]

  // Abstract page.
  // Each part is printed only if it has content, so a report without an
  // English abstract still gets its Zusammenfassung.
  if zusammenfassung != [] or abstract != [] [
    #if zusammenfassung != [] [
      #align(center)[
        #heading(outlined: false, numbering: none, text(0.85em, "ZUSAMMENFASSUNG"))
      ]
      #zusammenfassung
    ]

    #if abstract != [] [
      #align(center)[
        #heading(outlined: false, numbering: none, text(0.85em, "ABSTRACT"))
      ]
      #abstract
    ]

  ]

  if acknowledgments != [] [
    #align(center)[
      #heading(outlined: false, numbering: none, text(0.85em, "DANKSAGUNG"))
    ]
    #acknowledgments
  ]

  show outline.entry: it => {
    if(it.element.func() == heading) {
      if (it.level == 1) {
        block(
          above: 1.2em,
          link(it.element.location(), text(it, 13pt, black, weight: "bold"))
          
        )
      }

      if (it.level == 2) {
        block(
          above: 0.75em,
          link(it.element.location(), text(it, 13pt, black))
        )
      }
      if (it.level == 3) {
        block(
          above: 0.75em,
          link(it.element.location(), text(it, 13pt, black))
        )
      }
    } else {
      it
    }
  }

  show ref: it => {
   let element = it.element
   if element != none and element.func() == heading {
      if element.level >= 4 {
         link(element.location(), element.body)
      }else {
         it
      }
   }else {
      it
   }
}

  if show_outline {
    outline(
      depth: 3,
      indent: auto,
    )

    if front_matter != none {
      pagebreak()
      front_matter
    }

    //counter(page).update(3)
    pagebreak()
  }

  // Main body.
  set page(numbering: "1", number-align: center)
  set page(header: getHeader())
  counter(page).update(1)
  body
}
