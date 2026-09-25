# Bedienungsanleitung TeenAstro

Andere Sprachen: [English](Manuel_utilisateur_en.md) · [français](Manuel_utilisateur_fr.md)

Eine Anleitung für die Nacht. Sie beschreibt Handkontroller, Steuergerät, Nachführung, Goto, Ausrichtung und die mechanischen Einstellungen so, wie sie in der aktuellen Firmware vorhanden sind.

Sie folgt dem Aufbau der FS2-Anleitung von Astro-Electronic (Anschluss, Menüs, Ändern eines Wertes, Drehrichtung, Geschwindigkeiten, Goto, deutsche Montierung, Encoder, Besuchermodus) und stützt sich auf die Menüs des Handkontrollers, das Steuergerät und das Wiki der [TeenAstro-Gruppe](https://groups.io/g/TeenAstro/wiki/home).

Vor der ersten Nacht muss nicht alles gelesen werden. Die Kapitel 3, 5, 7 und 8 genügen für den Start. Eine Nacht an einer schon eingerichteten Montierung ist Kapitel 25. Der Rest dient der Mechanik, der feinen Ausrichtung und der Fehlersuche.

Die fett gesetzten Menünamen sind die Beschriftungen der deutschen Handkontroller-Firmware. Einige Zeilen bleiben in dieser Version auf Englisch: `Encoders`, `Auto Sync`, `Calibration`, `Pulse per deg`, `Silent`, `J2000`, `JNow`, `Alt Az`, `Show align. error`, `Wifi`, `Home`, `Park`, `Spiral`.

---

## 1. Woraus das System besteht

TeenAstro ist eine Steuerung für äquatoriale und altazimutale Montierungen. Sie wurde von einem FS2-Nutzer gebaut, der einen offenen Nachfolger wollte: dieselben Handgriffe in der Nacht (Handkontroller, Nachführung, Goto, Synchronisierung), mit Schrittmotoren, einer Zweistern-Ausrichtung und einer Verbindung zum Rechner.

| Teil | Aufgabe |
|------|---------|
| Steuergerät | Rechnet den Himmel, treibt die Motoren, speichert Montierung, Zeit und Ort |
| Handkontroller (SHC) | Anzeige, sieben Tasten, Kataloge, Einstellungen |
| WLAN-Schnittstelle | Brücke zwischen Steuergerät und Telefon oder Rechner |
| Fokusierer | Fokussiermotor, optional |
| TeenAstro-App | Übersicht, Planetarium, Goto |
| ASCOM-Treiber | Stellarium, Cartes du Ciel und andere ASCOM-Programme |

Das Steuergerät ist der einzige Ort, an dem die Position der Montierung geführt wird. Handkontroller, WLAN und Rechner schicken Befehle und zeigen die Antwort.

Zwei Parametersätze können nebeneinander liegen (Montierung 0 und Montierung 1). Der Wechsel erfolgt im Menü Montierung. Danach ist ein Neustart nötig.

---

## 2. Vorsicht

- Motorkabel nicht bei eingeschalteter Spannung stecken oder ziehen. Ein unter Spannung getrennter Schrittmotor kann den Treiber zerstören.
- Vor einer Änderung von Montierungsart, Getriebezahl, Schrittzahl oder Drehrichtung die Werte aus **Zeige Einst.** notieren oder mit TeenAstroConfig sichern. Ein falscher Satz macht aus einer laufenden Montierung eine, die nicht mehr trifft.
- Die Position **Home** beim Einschalten ist der mechanische Nullpunkt. Steht der Tubus nicht dort, sind die angezeigten Koordinaten falsch, bis eine Synchronisierung oder eine neue Ausrichtung erfolgt.
- Der Modus **Besucher** verbirgt die Einstellungen. Vor einer Einstellungssitzung wieder auf **Admin.** wechseln.
- Ein Werksreset löscht Zeit, Ort, Mechanik und Ausrichtung. Er verlangt eine Bestätigung.

Spannung, Stecker und Schaltplan hängen von der Platine ab (klassische Einheit, Mini, Tiny). Die Verdrahtung jeder Karte steht im Wiki der Gruppe, nicht in dieser Anleitung.

---

## 3. Der Handkontroller

Sieben Tasten: **Shift**, **Nord**, **Süd**, **Ost**, **West**, **F** und **f**.

Außerhalb eines Menüs bewegen Nord, Süd, Ost und West den Tubus. Die aktuelle Geschwindigkeit steht als Symbol oben in der Anzeige.

| Geste | Wirkung |
|-------|---------|
| Kurzer Druck auf Shift | Nächste Anzeigeseite |
| Shift gehalten + Ost | **Teleskop Aktion** |
| Shift gehalten + Nord | **Geschw. setzen** |
| Shift gehalten + Süd | **Anzeige** (aus, Kontrast) |
| Shift gehalten + West | **Teleskop-Einst.** |
| Shift gehalten + F | **Fokusierer Einst.** |
| Shift gehalten + f | **Fokusierer Aktion** |

Im Menü:

| Taste | Wirkung |
|-------|---------|
| Nord / Süd | In der Liste auf oder ab |
| West | Zeile öffnen oder Wert erhöhen |
| Ost | Zurück oder Wert verringern |
| F | Bestätigen |
| f oder Shift | Menü verlassen |

**Ergonomie** dreht den Handkontroller für Linkshänder: die Anzeige dreht sich um 180°, Nord/Süd und Ost/West tauschen die Plätze. Die Änderung gilt nach dem Neustart.

**Geschw. Knopf** (Langsam, Mittel, Schnell) stellt die Tastenwiederholung ein, nicht die Geschwindigkeit der Montierung.

Die Anzeige wird dunkler und schaltet sich nach einer Weile ab. **SHC Einst. → Anzeige** stellt Dimmen, Ausschalten und das OLED-Untermodell ein. Ein Druck auf Shift weckt die Anzeige. **Shift + Süd** schaltet sie auch sofort aus oder ändert den Kontrast (Min, Niedrig, Hoch, Max).

---

## 4. Die Anzeige lesen

Die Bilder stammen vom Emulator des Handkontrollers, französische Firmware 1.6.7. Die echte Anzeige hat 128×64 Punkte und ist hier vierfach vergrößert. Eine deutsche oder englische Fassung hat dieselbe Anordnung. Nur die Wörter ändern sich. Der französische Text auf den Bildern ist also die französische Firmware, nicht eine andere Seite.

### Beim Einschalten

Zuerst das Logo, dann die Versionen, Platine und Treiber, der Fokusierer wenn er antwortet, und zuletzt die Uhr, wenn kein GNSS verbunden ist.

![Logo TeenAstro](images/screens/boot_logo.png)

![Version des Handkontrollers](images/screens/boot_shc.png)

![Version des Steuergeräts](images/screens/boot_main.png)

![Platine und Treiber](images/screens/boot_pcb.png)

![Version des Fokusierers](images/screens/boot_focuser.png)

![Uhrzeit und Datum beim Start](images/screens/boot_clock.png)

Zeigt der Handkontroller **FEHLER** und **version**, passen die Firmware von Handkontroller und Steuergerät nicht zusammen. Beide aus derselben Veröffentlichung neu laden.

Zeigt er **Nicht verbunden**, ist die serielle Verbindung zum Steuergerät verloren. Der Handkontroller startet neu. Zuerst das Kabel des Handkontrollers prüfen.

### Die Seiten

Ein kurzer Druck auf Shift blättert zur nächsten Seite. Die Datei `libraries/TeenAstoCustomizations/TeenAstoCustomizations.h` legt fest, welche Seiten es gibt. Im Auslieferungszustand sind vier aktiv: Rektaszension und Deklination, Azimut und Höhe, Zeit, Fokusierer. Die übrigen erscheinen, wenn der Kommentar vor der passenden Zeile entfernt wird (`HA_PAGE`, `PUSH_PAGE`, `AXIS_STEP_PAGE`, `AXIS_DEG_PAGE`). Während einer Ausrichtung ersetzt der Ausrichtungsbildschirm die aktuelle Seite.

Bei Home auf einer äquatorialen Montierung steht die Deklination am Pol, und das Home-Symbol leuchtet.

![Rektaszension und Deklination](images/screens/radec.png)

Standardseite. **RA** ist die Rektaszension, **Dec** die Deklination. Oben links: WLAN nicht verbunden, dann die Handgeschwindigkeit (hier zwei Balken, langsam). Oben rechts: Home.

![Stundenwinkel](images/screens/hadec.png)

Optionale Seite. **HA** ist der Stundenwinkel, **Dec** die Deklination. Daran sieht man, auf welcher Seite des Meridians das Objekt steht.

![Azimut und Höhe](images/screens/altaz.png)

Standardseite. **Az.** ist das Azimut, **Alt.** die Höhe. Bei äquatorialem Home zeigt der Tubus zum Pol, die Höhe hängt also von der Breite des Standorts ab.

![Push-to-Seite](images/screens/push.png)

Optionale Seite. Ohne Encoder bleibt sie leer: nur die Statuszeile wird gezeichnet. Mit aktiven Encodern zeigt sie den Restweg jeder Achse bis zum Ziel.

![Zeit](images/screens/time.png)

Standardseite. **UTC** ist die Weltzeit, **LST** die lokale Sternzeit.

![Achsen in Schritten](images/screens/axis_steps.png)

Optionale Seite. Die beiden Zahlen sind die Schrittzähler von Achse 1 und Achse 2. Sie zeigen, dass ein Motor dreht. Es sind keine Himmelskoordinaten.

![Achsen in Grad](images/screens/axis_deg.png)

Optionale Seite. Das erste Paar ist der Motorwinkel der Achsen 1 und 2. Das zweite Paar sind dieselben Winkel nach Korrektur (Spiel, Modell). Ohne Encoder heißen alle vier Zeilen Ax1 und Ax2.

![Fokusierer](images/screens/focuser.png)

Standardseite, wenn ein Fokusierer antwortet. **F Position** ist die Fokusposition, **F Speed** seine Geschwindigkeit. Antwortet er nicht, zeigt die Seite **Focuser** und **Nicht verbunden**.

### Die Menüs

Ein Menü öffnet sich, indem man Shift hält und dann eine Richtung drückt. Shift oder **f** verlässt es. Ost geht zurück. West oder **F** öffnet die hervorgehobene Zeile. Nord und Süd verschieben die Hervorhebung.

Die Wörter auf den Bildern sind die französische Firmware. Auf Deutsch heißen sie **Teleskop Aktion**, **Geschw. setzen**, **Anzeige**, **Teleskop-Einst.** und **Handkontroller**.

![Teleskop Aktion, Shift und Ost](images/screens/menu_action.png)

**Goto**, **Synchro** und **Ausrichten** sind die ersten Zeilen. Es folgen Getriebe-Check, Nachführung, Pierseite, Speichern der Koordinaten, Sperre und Spirale. Eine geparkte Montierung bietet nur das Entparken.

![Geschwindigkeit setzen, Shift und Nord](images/screens/menu_speed.png)

Die hervorgehobene Zeile ist die aktuelle Geschwindigkeit. Die fünf Stufen sind Guiding, Langsam, Mittel, Schnell und Max. Die Balken links auf der Hauptseite folgen dieser Wahl: ein Balken für Guiding, dann zwei, drei, vier, und fünf für Max.

![Anzeige, Shift und Süd](images/screens/menu_display.png)

**Ausschalten** löscht die Anzeige sofort. **Kontrast** bietet Min, Niedrig, Hoch und Max. Die OLED-Schrift hat nicht jeden Akzent, deshalb kann ein französischer Titel seinen Akzent verlieren.

![Teleskop-Einstellungen, Shift und West](images/screens/menu_settings.png)

Im Administratormodus: **Handkontroller**, **Zeit & Ort**, **Park und Home**, dann Montierung, die Informationen des Steuergeräts und WLAN. Im Besuchermodus enthält dieses Menü nur **Rechte**.

![SHC Einst.](images/screens/menu_shc.png)

Der Titel dieses Bildschirms ist **SHC Einst.** **Rechte** wählt Administrator oder Besucher. **Anzeige** stellt Dimmen, Tiefschlaf und das OLED-Modell ein. **Geschw. Knopf** stellt die Tastenwiederholung ein (langsam, mittel, schnell), nicht die Geschwindigkeit der Montierung. Danach kommen Ergonomie und das Rücksetzen des Handkontrollers.

### Die Symbole

Die Statuszeile ist eine Reihe von Symbolen mit 16×16 Punkten. Links: WLAN, dann die Geschwindigkeit wenn die Motoren unter Spannung sind, dann der Shift-Pfeil solange die Taste gehalten wird. Rechts: der Zustand der Montierung. Nachführung, Schwenk, Home und Park teilen sich einen Platz: das Wichtigste gewinnt. Fehler kommen daneben dazu.

**WLAN.** Ein gefülltes Gehäuse bedeutet verbunden. Nur der Umriss bedeutet, dass die Schnittstelle an ist, aber kein Link besteht. Die vier Zeichnungen sind die drei Stationsprofile (0, 1, 2) und der Zugangspunkt.

| Verbunden | Nicht verbunden | Rolle |
|-----------|-----------------|-------|
| ![](images/icons/wifi_sta0.png) | ![](images/icons/wifi_sta0_nc.png) | Station, Profil 0 |
| ![](images/icons/wifi_sta1.png) | ![](images/icons/wifi_sta1_nc.png) | Station, Profil 1 |
| ![](images/icons/wifi_sta2.png) | ![](images/icons/wifi_sta2_nc.png) | Station, Profil 2 |
| ![](images/icons/wifi_ap.png) | ![](images/icons/wifi_ap_nc.png) | Zugangspunkt |

**Handgeschwindigkeit.** Die Zahl der Balken ist die in Geschw. setzen gewählte Stufe. Das Symbol erscheint nur, wenn die Motoren unter Spannung sind.

| | Stufe |
|---|-------|
| ![](images/icons/GUIDINGSP.png) | Guiding |
| ![](images/icons/SLOWSP.png) | Langsam |
| ![](images/icons/MEDIUMSP.png) | Mittel |
| ![](images/icons/FASTSP.png) | Schnell |
| ![](images/icons/MAXSP.png) | Max, die Goto-Geschwindigkeit |

**Wo der Tubus steht.**

| | Bedeutung |
|---|-----------|
| ![](images/icons/home.png) | In der Home-Position |
| ![](images/icons/parked.png) | Geparkt |
| ![](images/icons/parking.png) | Parken läuft |
| ![](images/icons/parkingFailed.png) | Parken ist fehlgeschlagen |
| ![](images/icons/no_tracking.png) | Nachführung aus (zwei senkrechte Balken) |
| ![](images/icons/tracking.png) | Nachführung an, Rate nicht angegeben (Dreieck) |
| ![](images/icons/tracking_star.png) | Sternzeit |
| ![](images/icons/tracking_sun.png) | Sonne |
| ![](images/icons/tracking_moon.png) | Mond |
| ![](images/icons/tracking_target.png) | Nachführung mit der Rate des Objekts (Komet, Drift) |
| ![](images/icons/tracking_ra.png) | Nachführung mit Korrektur nur in Rektaszension |
| ![](images/icons/tracking_both.png) | Nachführung mit Korrektur auf beiden Achsen |
| ![](images/icons/slewing_eq.png) | Äquatoriales Goto |
| ![](images/icons/slewing_altaz.png) | Altazimutales Goto |
| ![](images/icons/slewing_flip.png) | Umschwenken am Meridian |
| ![](images/icons/sleewing.png) | Schwenk läuft, Art nicht angegeben |
| ![](images/icons/E.png) | Pierseite Ost |
| ![](images/icons/W.png) | Pierseite West |

Auf das Nachführdreieck werden Stern, Sonne, Mond oder Ziel gelegt, danach eine 1 oder eine 2, wenn eine Nachführkorrektur aktiv ist. Während eines Goto ersetzt das Schwenksymbol dieses Dreieck.

**Ausrichtung, Guiding, Spirale.**

| | Bedeutung |
|---|-----------|
| ![](images/icons/align1.png) | Ausrichtung, Stern 1 |
| ![](images/icons/align2.png) | Ausrichtung, Stern 2 |
| ![](images/icons/align3.png) | Ausrichtung, Stern 3 oder folgender |
| ![](images/icons/Aligned.png) | Ein Ausrichtungsmodell ist gespeichert |
| ![](images/icons/Spiral.png) | Spiralsuche läuft |
| ![](images/icons/guiding_.png) | Rahmen eines Guiding-Impulses |
| ![](images/icons/guide_n.png) | Impuls nach Norden |
| ![](images/icons/guide_s.png) | Impuls nach Süden |
| ![](images/icons/guide_e.png) | Impuls nach Osten |
| ![](images/icons/guide_w.png) | Impuls nach Westen |
| ![](images/icons/recenter_base.png) | Neu zentrieren (Zielscheibe) |
| ![](images/icons/atrate_base.png) | Guiding mit Nachführgeschwindigkeit (Fadenkreuz) |

Die Guiding-Pfeile liegen über dem Rahmen, der Zielscheibe oder dem Fadenkreuz, je nachdem ob die Montierung einen ST-4-Impuls, ein Neuzentrieren oder ein Guiding mit Nachführgeschwindigkeit erhält.

**GNSS, Sperre, Shift.** Das GNSS-Symbol erscheint nur bei Home oder Park.

| | Bedeutung |
|---|-----------|
| ![](images/icons/GNSS.png) | Zeit und Ort synchronisiert |
| ![](images/icons/GNSST.png) | Nur die Zeit |
| ![](images/icons/GNSSL.png) | Nur der Ort |
| ![](images/icons/shift.png) | Shift wird gehalten |
| ![](images/icons/Lock__.png) | Gesperrt, ohne zu sagen was |
| ![](images/icons/lock_t.png) | Teleskop gesperrt |
| ![](images/icons/lock_f.png) | Fokusierer gesperrt |
| ![](images/icons/lock_both.png) | Teleskop und Fokusierer gesperrt |

**Fehler.** Sie ersetzen den normalen Zustand rechts oder kommen daneben dazu. Der Kurztext steht im Symbol selbst.

| | Bedeutung |
|---|-----------|
| ![](images/icons/ErrA1.png) | Grenze der Achse 1 |
| ![](images/icons/ErrA2.png) | Grenze der Achse 2 |
| ![](images/icons/ErrHo.png) | Unter dem Horizont |
| ![](images/icons/ErrMe.png) | Meridiangrenze |
| ![](images/icons/ErrUp.png) | Unter dem Pol |
| ![](images/icons/ErrMf.png) | Motorfehler |

---

## 5. Einen Wert ändern

Das Prinzip ist überall dasselbe, wie beim FS2.

1. Menü öffnen (in der Regel Shift + West für Einstellungen, Shift + Ost für die Nacht).
2. Mit Süd zur Zeile, dann mit West oder F öffnen.
3. Bei einer Liste (Montierungsart, Drehrichtung, Mikroschritt) verschieben Nord und Süd die Markierung, F speichert.
4. Bei einer Zahl (Getriebezahl, Spiel, Breite) erhöht West, Ost verringert, F speichert. Die Schrittweite ist auf diesem Bildschirm fest.
5. **Gesetzt** bestätigt das Schreiben. **FEHLER** oder ein fehlgeschlagener Befehl bedeutet, dass der Wert nicht übernommen wurde.
6. Ost oder f geht eine Ebene zurück, ohne den gerade bearbeiteten Wert zu ändern.

Manche Änderungen verlangen einen Neustart: Montierungsart, Wahl von Montierung 0 oder 1, Aktivieren der Motoren oder Encoder, Ergonomie, WLAN. Die Anzeige zeigt dann **Taste drücken** / **für Neustart**. Eine Taste drücken und neu starten lassen. Die Spannung nicht mitten in dieser Meldung abschalten.

---

## 6. Erstes Einschalten

Beim ersten Einschalten nimmt das Steuergerät an, es stehe auf der Homeposition. Das ist noch keine auf den Himmel ausgerichtete Montierung. Es ist nur der mechanische Nullpunkt.

### Was vor dem Zeigen einzustellen ist

1. **Montierungs-Art**: Deutsche Monti., Äquatorial Gabel, Alt-Azimutal oder Alt-Azimutal Gabel.
2. **Getr. Zahl**, **Schr. pro Rot.** und **Mikroschritt** jedes Motors (Kapitel 9).
3. Die **Drehrichtung** (Kapitel 8).
4. **Ort** (Breite, Länge, Höhe) und **Zeit** (Kapitel 10).

Ohne Ort und Zeit kann die Sternzeit-Nachführung laufen, aber äquatoriale Koordinaten und Kataloge passen nicht zum Himmel.

### Homeposition, Voreinstellung

- Äquatoriale Montierung: Tubus zum Himmelspol. Bei einer deutschen Montierung zeigt die Gegengewichtsstange nach unten.
- Altazimutale Montierung: Höhe 0° (Tubus waagerecht) und Azimut 180° (Tubus nach Süden). Die Basis muss in der Waage stehen.

Den Tubus **vor** dem Einschalten in diese Lage bringen, oder einschalten, mit den Tasten nach Home fahren und die Position speichern (Kapitel 11).

Für einen ersten Versuch kann Home entfallen: den Tubus von Hand auf ein auffälliges Objekt stellen (Mond, Planet, heller Stern), **Synchronisierung**, danach Goto. Die Zweistern-Ausrichtung dagegen startet von der Homeposition oder von einem Stern, dessen Montierungsseite angegeben wird.

### Kurze Prüfung, wie sie die Gruppe vorschlägt

1. Goto auf **Home**, dann bei Bedarf mit gelösten Klemmen den Tubus zum Pol richten.
2. Goto auf einen hellen Stern, zentrieren, **Synchronisierung**.

Das genügt für einen visuellen Abend, wenn die mechanische Polausrichtung schon stimmt. Für ein Modell, das einen Fehler der Polausrichtung aufnimmt, die Ausrichtung in Kapitel 15 verwenden.

---

## 7. Montierungsart, Refraktion, Pol

**Teleskop-Einst. → Montierung**

| Zeile | Inhalt |
|-------|--------|
| Montierung | Wählt Satz 0 oder Satz 1. Neustart |
| Montierungs-Art | Deutsche Monti., Äquatorial Gabel, Alt-Azimutal, Alt-Azimutal Gabel. Neustart |
| Motoren | Mechanik, Geschwindigkeiten, Nachführung |
| Encoders | Positionsgeber |
| Grenzen | Horizont, Zenit, Achsen, Meridian |
| Refraktion | Siehe unten |
| Strichplatte | Helligkeit des Polsucher-Fadenkreuzes, wenn der Ausgang verdrahtet ist |

**Refraktion → Goto**: AN oder AUS. Eingeschaltet geht die atmosphärische Refraktion in den Schwenk ein (Saemundsson hin, Bennett zurück).

**Refraktion → Pol-Ausrichtung** (nur äquatoriale Montierungen):

- **Scheinbarer Pol**, wenn ein Polsucher benutzt wird. Er zeigt den Pol dort, wohin die Refraktion ihn verschiebt.
- **Wahrer Pol**, wenn die Einnordung auf die wahre Polachse zielt, bereits um die Refraktion korrigiert.

Bei einer altazimutalen Montierung wird nur die Goto-Refraktion angeboten: es gibt keinen mechanischen Pol zur Auswahl.

---

## 8. Drehrichtung

Der Test erfolgt bei langsamer oder mittlerer Geschwindigkeit, Tubus ungefähr auf Home, Blick durch Sucher oder Okular.

**Deutsche Montierung**

- Süd muss den Tubus nach Westen schicken. Fährt er nach Osten, Drehung der Achse 1 umkehren und erneut prüfen.
- Ost muss den Tubus nach Osten schicken. Sonst die betroffene Achse umkehren und noch einmal prüfen.

**Äquatoriale Gabel**

- Süd muss den Tubus nach Süden schicken. Sonst Achse 2 umkehren.
- Ost muss den Tubus nach Osten schicken. Sonst Achse 1 umkehren.

**Altazimutal**: Ost und West steuern den Azimut (Achse 1), Nord und Süd die Höhe (Achse 2). Der Tubus muss in die Richtung fahren, die die Taste nennt.

Die Einstellung steht unter **Montierung → Motoren → Motor 1** oder **Motor 2 → Drehung**: **Direkt** oder **Rückwärtig**.

Achse 1 ist Rektaszension oder Azimut. Achse 2 ist Deklination oder Höhe.

---

## 9. Motoren, Getriebe, Strom, Spiel

**Montierung → Motoren**, Motoren aktiv:

Zeige Einst., Motor 1, Motor 2, Beschl., Geschwindigkeit, Nachführung, Warte Zeit, Deaktivieren.

**Zeige Einst.** zeigt für jede Achse Drehrichtung, Getriebezahl, Schritte pro Umdrehung, Mikroschritt, Spiel in Bogensekunden sowie niedrigen und hohen Strom. Diese Seiten durchgehen und die Werte notieren, bevor etwas geändert wird.

### Getriebezahl

Das ist die Zahl der Motorumdrehungen für eine Achsumdrehung, Zähne eingeschlossen.

Beispiel: Rad mit 360 Zähnen und Schnecke, ohne weitere Untersetzung: Getriebezahl 360. Eine Riemenscheibe 2:1 vor der Schnecke macht 720. Ein Planetengetriebe 5:1 auf derselben Achse macht 1800.

Der zulässige Wert geht von 1 bis 60000, mit drei Nachkommastellen. Das Gesamtübersetzungsverhältnis eintragen, nicht nur die Zähnezahl des Rades, wenn eine weitere Stufe vorhanden ist.

**Getriebe-Check** (Kapitel 19) misst dieses Verhältnis am Himmel und schlägt einen korrigierten Wert vor.

### Schritte pro Umdrehung und Mikroschritt

**Schr. pro Rot.** ist die Zahl der Vollschritte des Motors, meist 200 oder 400. Die Eingabe geht von 20 bis 400.

**Mikroschritt**: 2, 4, 8, 16 (~256), 32, 64, 128, 256. Die Gruppe behält auf TMC-Treibern am häufigsten **16 (~256)**: 16 echte Mikroschritte, interne Interpolation auf 256. Höher als 16 ohne Bedarf senkt die maximale Schwenkgeschwindigkeit stärker, als es die Nachführung verbessert.

### Strom

**niedrige Str.** gilt für langsame Geschwindigkeiten. **hohe Str.** gilt für schnelle Schwenks. Die Eingabe erfolgt in Schritten von 100 mA Spitze, von 200 mA bis 2800 mA.

Den Strom erhöhen, wenn der Motor beim Goto aussetzt oder unter Last brummt. Ihn senken, wenn der Motor ohne Nutzlast warm wird. Der hohe Strom muss innerhalb der Grenzen von Motor und Treiber bleiben.

### Spiel

**Getr. Spiel** ist der Ausgleich beim Richtungswechsel, in Bogensekunden, von 0 bis 999.

**Spiel Geschw.** stellt die Geschwindigkeit dieses Ausgleichs ein, von 16 bis 64.

Das Spiel erst einstellen, wenn Getriebezahl und Drehrichtung stimmen. Zu viel Spiel lässt den Stern bei jeder Umkehr springen. Zu wenig lässt einen sichtbaren Versatz, wenn man mit den Tasten zurückfährt.

### Leiser Betrieb

**Silent** AN nutzt den leisen Modus des Treibers (StealthChop bei TMC). AUS bevorzugt das Drehmoment bei hoher Geschwindigkeit. Setzt das Goto nur bei Silent aus, für die Versuche auf AUS stellen, den hohen Strom anheben und Silent danach erneut versuchen.

### Beschleunigung

**Beschl.** ist die Strecke in Grad, auf der die Montierung die maximale Schwenkgeschwindigkeit erreicht. Von 0,1° bis 25°. Ein zu kleiner Wert rüttelt die Montierung. Ein zu großer Wert verlängert jedes Goto.

### Wartezeit

**Warte Zeit** ist die Pause am Ende eines Schwenks, bevor die Nachführung wieder einsetzt. Sie lässt die Schwingungen abklingen.

### Motoren abschalten

**Deaktivieren** schaltet die Motoransteuerung aus und verlangt einen Neustart. Nützlich, um den Tubus von Hand zu bewegen, ohne gegen das Haltemoment zu arbeiten. **Aktivieren** schaltet sie wieder ein. Solange die Motoren aus sind, besteht das Menü nur aus dieser Zeile.

---

## 10. Zeit, Ort, GNSS

**Teleskop-Einst. → Zeit & Ort**

### Zeit

- **Uhr**: angezeigte bürgerliche Zeit.
- **Zeitzone**: Abstand zu UTC, einschließlich Sommerzeit. In Mitteleuropa: +1 h im Winter, +2 h im Sommer.
- **Datum**
- **GNSS Zeit**: Zeit vom Satellitenempfänger, falls einer vorhanden ist.

Das Steuergerät leitet daraus die lokale Sternzeit ab. Eine um eine Minute falsche Uhr verschiebt das Zeigen um etwa 15' in Rektaszension.

### Ort

Drei Orte können gespeichert werden.

- **Breitengrad**: positiv im Norden, negativ im Süden.
- **Längengrad**: das Vorzeichen ist das der Eingabemaske. Es nach der ersten Synchronisierung an einem bekannten Objekt prüfen. Ein umgekehrter Längengrad fällt sofort auf: jedes Goto landet azimutal auf der Gegenseite.
- **Geo. Höhe**: Höhe des Ortes in Metern. Sie geht in die Refraktion ein.
- **Ort wählen**: der aktive Ort.

### GNSS-Synchronisierung

**Sync mit GNSS** übernimmt Zeit und Ort des Empfängers ins Steuergerät. Ohne Antenne oder in einem Gebäude zeigt die Anzeige **KEIN GNSS**. Eine automatische Synchronisierung wird angeboten, wenn die Montierung auf Home oder Park steht und der Empfänger einen Fix hat.

---

## 11. Home und Park

Die **Homeposition** ist der Nullpunkt der Achsen. Die **Parkposition** ist der Ort, an dem der Tubus am Ende der Nacht abgestellt wird, während die Motoren ihren Winkel noch kennen, damit der Himmel beim nächsten Einschalten ohne neue Einnordung wiedergefunden wird.

| | Home | Park |
|---|------|------|
| Aufgabe | Mechanischer Nullpunkt, Start der Ausrichtung | Abstellen |
| Äquatorial | Zum Pol, Gegengewicht bei einer deutschen Montierung unten | Dort, wo der Tubus sicher steht |
| Altazimutal, Voreinstellung | Höhe 0°, Azimut 180° (Süden) | Nach Wahl |

**Teleskop-Einst. → Park und Home**

- **Setze Parkposition**: die aktuelle Lage wird Park.
- **Setze Homeposition**: die aktuelle Lage wird Home. Nur verwenden, wenn der Tubus wirklich in der Lage steht, die der Nullpunkt sein soll.
- **Reset HomePosition**: zurück zur Werkshome der Montierungsart.

**Teleskop Aktion → Goto → Home** oder **Park** fährt den Tubus dorthin. **Synchronisierung → Home** oder **Park** erklärt, der Tubus stehe schon dort, ohne ihn zu bewegen. Nur für den Fall, dass er von Hand dorthin gestellt wurde.

Ist die Montierung geparkt, bietet das Aktionsmenü nur **Unpark**. Vor jeder Bewegung entparken.

Das Home- oder Parksymbol rechts in der Anzeige bestätigt den Zustand. Fehlt es, obwohl der Tubus auf Home zu stehen scheint, sind gespeicherte und wirkliche Lage auseinandergelaufen: entweder den Tubus zurückfahren oder Home neu setzen.

---

## 12. Geschwindigkeiten

Zwei verschiedene Stellen:

- **Shift + Nord → Geschw. setzen** wählt die Tastengeschwindigkeit für die Sitzung: Guiding, Langsam, Mittel, Schnell, Max. Die hervorgehobene Zeile ist die aktive Stufe. Die Balken stehen in Kapitel 4. Das Bild zeigt die französische Firmware (**Réglage Vitesse**).
- **Motoren → Geschwindigkeit** legt fest, was diese fünf Stufen bedeuten, und welche beim Einschalten gilt.

![Geschwindigkeit setzen](images/screens/menu_speed.png)

| Stufe | Einstellung | Bereich |
|-------|-------------|---------|
| Guiding | Bruchteil der Sternzeitgeschwindigkeit | 0,10× bis 1,00× |
| Langsam, Mittel, Schnell | Vielfaches der Sternzeitgeschwindigkeit | 1× bis 255× |
| Max | Goto-Geschwindigkeit | 60× bis 3600×, in Schritten von 60 |
| Standard Geschw. | Stufe beim Einschalten | eine der fünf |

Die Guiding-Geschwindigkeit ist auch die Geschwindigkeit der ST-4-Buchse.

Zum Zentrieren eines Sterns: Schnell oder Mittel bis in den Sucher, Langsam im Okular, Guiding, um ihn in die Mitte zu legen, ohne darüber hinauszufahren.

---

## 13. Die Nachführung

**Teleskop Aktion → Nachführung**

Ist die Nachführung aus, gibt es nur die Zeile **Nachführ. starten**.

Läuft sie:

- **Nachführ. stoppen**
- **Stern**: Sterne
- **Mond**: Mond
- **Sonne**: Sonne
- **Ziel/Objekt**: gespeicherte Rate für einen Kometen oder ein driftendes Objekt

Die Rate lässt sich während eines Schwenks nicht ändern. Die Anzeige meldet **Nachführung läuft** / **nicht änderbar**.

### Drift (Kometen, langsame Objekte)

**Motoren → Nachführung → Drift Geschw.**

- **Rektaszension**: in Zeitsekunden je Sternzeitintervall (angezeigte Einheit `s/SI`), von −2 bis +2.
- **Deklination**: in Bogensekunden je Sternzeitintervall (`"/SI`), von −2 bis +2.

Beide Werte nach der Ephemeride setzen, die Nachführung starten, dann **Ziel/Objekt** wählen. Für Sterne wieder auf Sternzeit zurück. Drift null auf beiden Achsen zusammen mit der Zielrate ergibt wieder die Sternzeit-Nachführung.

### Refraktion auf der Nachführung

**Motoren → Nachführung → Refraktion**: AN oder AUS. Getrennt von der Goto-Refraktion. Sie korrigiert die Nachführgeschwindigkeit nach der Höhe des Objekts.

### Nachführkorrektur

**Motoren → Nachführung → Nachführ. Korr.**, bei äquatorialen Montierungen:

- **Rektaszension**: Ausrichtungs- und Refraktionskorrektur wirken nur auf die Stundenachse.
- **Beide**: beide Achsen werden korrigiert.

Bei einer altazimutalen Montierung erfolgt die Korrektur immer auf beiden Achsen, und diese Zeile erscheint nicht. Das Nachführsymbol ändert sich, wenn die Korrektur eingeschaltet ist (eine Achse oder beide).

Die Gruppe empfiehlt die Korrektur auf beiden Achsen, sobald eine Zweistern-Ausrichtung gespeichert ist, und nur auf der Rektaszension, wenn die Einnordung schon gut ist und das Modell keine Drift in Deklination einbringen soll.

---

## 14. Goto, Synchronisierung, Kataloge

**Shift + Ost → Teleskop Aktion**, Montierung entparkt.

| Zeile | Aufgabe |
|-------|---------|
| Goto | Fährt den Tubus auf das Ziel |
| Pushto | Zeigt die Richtung, ohne Motoren, wenn Encoder aktiv sind |
| Synchronisierung | Erklärt, das gewählte Objekt stehe in der Okularmitte |
| Ausrichten | Zweistern-Modell |
| Getriebe-Check | Misst die Getriebezahl |
| Nachführung | Start, Stopp, Wahl der Rate |
| Montierungs-Seite | Seite angeben, deutsche Montierung |
| Speichern RADEC | Merkt die aktuelle Lage als Benutzerziel |
| Sperre | Sperrt die Aktionen bis Entsperren |
| Spiral | Spiralsuche |

Die Menüs **Goto** und **Synchronisierung** bieten:

Kataloge, Sonnensystem, Koordinaten, Benutzer definiert, Home, Park.

Goto fügt **Umschw.** hinzu.

Push-to bietet Kataloge, Sonnensystem, Koordinaten und Benutzer definiert, und nur wenn Encoder aktiv sind. Sonst zeigt die Anzeige **Encoders** / **Nicht verbunden**.

### Sonnensystem

Sonne, Merkur, Venus, Mars, Jupiter, Saturn, Uranus, Neptun, Mond. Die Positionen sind scheinbare, für den aktuellen Ort und die aktuelle Zeit.

Die Sonne nur mit einem geeigneten Filter über die volle Öffnung anfahren. Die Steuerung weiß nicht, ob ein Filter eingesetzt ist.

### Koordinaten

- **J2000**: Rektaszension und Deklination des Äquinoktiums 2000.0, wie in den meisten gedruckten Katalogen.
- **JNow**: scheinbare Koordinaten des Datums.
- **Alt Az**: Azimut und Höhe.
- **N, S, O, W**: die vier Himmelsrichtungen am Horizont (Höhe 0°). Praktisch, um Richtung und Waage einer altazimutalen Montierung zu prüfen.

Das Steuergerät wendet Präzession, Nutation und Aberration zwischen J2000 und dem Datum an, danach die Umrechnung von äquatorial nach horizontal aus Breite und Sternzeit.

### Kataloge

Die Liste hängt von der im Handkontroller geladenen Firmware ab. In der Praxis sind es die hellen Sterne, Messier und je nach Übersetzung NGC, IC, Caldwell, Herschel, Collinder, Doppelsterne (STF, STT) und Veränderliche (GCVS). Die deutsche Firmware nennt den Sternkatalog «leuchtende Sterne».

### Filter

**Goto → Kataloge → Filters**. Der Titel des Bildschirms ist **FiltersElaub**, so wie die Firmware ihn schreibt. Die Zeilennamen bleiben auf Englisch. Ein **+** auf beiden Seiten einer Zeile bedeutet, dass dieser Filter gilt. **Grundstellung Filters** hebt alle auf.

| Zeile | Was sie behält |
|-------|----------------|
| Above Horizon | Die Mindesthöhe. **Filter Horizont** bietet **Über Horizont**, dann **Über 10 deg.**, **Über 20 deg.**, und so weiter bis **Über 70 deg.** |
| Constellation | Ein Sternbild. **Filter bei Kon** beginnt mit **Alle**, dann die Abkürzungen: Her für Herkules, Lyr für Leier, And für Andromeda, Ori für Orion |
| Type | Ein Typ des tiefen Himmels. **Filter bei Typ** beginnt mit **Alle**. Die Namen sind die der Datenbank: Galaxy, Globular Clstr, Planetary Nebula, Open Cluster, Nebula |
| Magnitude | Die schwächste noch gezeigte Helligkeit. **Filter Magnitude** bietet **Alle**, dann 10, 11, 12, 13, 14, 15 und 16 |

Objekte unter dem Horizont fehlen schon. **Über Horizont** ändert daher wenig. **Über 30 deg.** behält nur, was hoch genug für ein bequemes Okular steht, weg von Bäumen und schlechtem Seeing.

Die Magnitude läuft entgegen der Intuition: eine kleine Zahl ist ein helles Objekt. **10** lässt alles weg, was Magnitude 10 oder schwächer ist. **Alle** schneidet nichts. Unter hellem Himmel mit einem kleinen Fernrohr genügen 10 oder 11. 16 lohnt nur, wenn Katalog und Himmel mitgehen.

**Type** ändert die Liste nur in einem Katalog des tiefen Himmels (Messier, NGC und die anderen). Bei den leuchtenden Sternen entfernt er nichts.

Wurde der Handkontroller mit Doppelsternen oder Veränderlichen übersetzt, kommen zwei weitere Zeilen, weiterhin auf Englisch:

- **Dbl* Min Sep.** und **Dbl* Max Sep.**: Winkelabstand, von 0,2" bis 100". Das Minimum muss unter dem Maximum bleiben, sonst zeigt die Anzeige **Min Sep must** / **be < Max Sep.**
- **Var* Max Per.**: größte Periode, von 0,5 Tag bis 100 Tage. Eine Veränderliche mit längerer Periode wird ausgeblendet. **Off** hebt den Filter auf.

Wenn ein Filter wirklich Objekte entfernt, steht der Katalogtitel zwischen Ausrufezeichen, zum Beispiel **!Goto Messier!**. Bleibt nichts übrig, sagt die Anzeige **No Object**: eine Stufe weiter öffnen, oder **Grundstellung Filters**.

Nord und Süd blättern die verbleibenden Objekte. F startet Goto oder Synchronisierung, je nach dem Menü, aus dem man kommt. Ein vollständiges Beispiel, M13 dann M31, steht in Kapitel 25.

### Zentrieren, dann synchronisieren

1. Goto auf das Objekt.
2. Im Okular das Objekt mit den Tasten in die Mitte bringen, langsam, dann Guiding.
3. **Synchronisierung** auf dasselbe Objekt.

Die Synchronisierung setzt die Position neu. Sie baut kein Ausrichtungsmodell: ein einzelnes Objekt lässt den Fehler bestehen, sobald man sich entfernt. Für ein Modell **Ausrichten** verwenden.

**Speichern RADEC** behält die aktuellen Koordinaten. **Goto → Benutzer definiert** kehrt dorthin zurück.

### Spirale

**Spiral** fragt ein Gesichtsfeld ab, von 1' bis 3°. Der Tubus beschreibt eine Spirale dieses Durchmessers, um ein Objekt zu finden, das knapp neben dem Feld gelandet ist. Ein langes Shift stoppt die Bewegung, wie bei einem Goto.

### Sperre

**Sperre** reduziert das Aktionsmenü auf **Entsperren**. Die Richtungstasten bleiben benutzbar. Praktisch, wenn der Handkontroller jemandem gegeben wird, der kein Goto auslösen soll.

---

## 15. Ausrichtung

Die Ausrichtung berechnet die Abbildung zwischen Himmel und Achsen. Das Verfahren ist das von Taki (zwei gemessene Richtungen), ergänzt um die nächstliegende echte Drehung im Sinne der kleinsten Quadrate. Das Ergebnis ist eine 3×3-Matrix. Sie dient dem Zeigen, der Synchronisierung, der korrigierten Nachführung und der Höhenkontrolle.

**Teleskop Aktion → Ausrichten**

Solange kein Modell gespeichert ist, äquatoriale Montierung:

- **2 Sterne**
- **2 Sterne Mech.**
- **Computer Ausrichtung**

Auf einer altazimutalen Montierung wird die mechanische Zeile nicht angeboten.

Steht ein Modell, kommen **Speichern**, **Löschen** und **Show align. error** hinzu.

### 2 Sterne, von Home

1. Der Tubus steht auf der Homeposition. Die Anzeige erinnert: **Die Montierung muss auf die Homeposition gesetzt werden.**
2. **Home** wählen, wenn die Anzeige den Modus abfragt.
3. Das Steuergerät nimmt den Start an. Den ersten Stern aus der Liste wählen (benannte Sterne, über dem Horizont).
4. Der Tubus fährt. **Fahre zu**, dann **Neuzentrieren**.
5. Den Stern zentrieren. Ein langer Druck auf Shift übernimmt den Stern (**Stern hinzugefügt**).
6. Den zweiten Stern wählen, weit vom ersten in Stundenwinkel und Deklination. Dasselbe Zentrieren, derselbe lange Druck.
7. Bei Erfolg wird das Modell gerechnet. **Speichern** schreibt es in den Speicher. Ohne Speichern geht es beim nächsten Park oder beim Abschalten verloren, je nachdem ob geparkt wird: das Wiki der Gruppe erinnert daran, am Ende der Prozedur zu parken, um das Ergebnis zu behalten. **Speichern** im Menü Ausrichten schreibt es ausdrücklich.

**Löschen** vergisst das Modell. Die Montierung kehrt zur Annahme zurück, sie sei mechanisch richtig und starte von Home.

### 2 Sterne, von einem Stern

Wenn Home nicht erreichbar ist (der Tubus steht schon am Himmel):

1. **Stern** statt **Home** wählen.
2. Die **Montierungs-Seite** angeben (Ost oder West). Bei Gabel oder Altazimut die Seite angeben, die der wirklichen Lage entspricht.
3. **Synchronisierung** auf einen zentrierten Stern. Dieser Stern wird die erste Referenz.
4. Die Prozedur geht mit dem zweiten Stern weiter, wie oben.

### 2 Sterne Mech.

Nur für deutsche Montierung und Gabel. Derselbe Ablauf, aber die Rechnung bleibt mit dem mechanischen Pol der Montierung verträglich. Zu verwenden, wenn die Einnordung die Referenz ist und das Modell den Pol nicht verdrehen soll. Der Start ist weiterhin **Home** oder **Stern**.

### Vom Rechner

Die Rechner-Ausrichtung wartet auf Sterne, die App oder Programm über das Protokoll schicken. Die Anzeige wechselt zu **Fern-Ausricht.** und zeigt den Namen des verlangten Sterns. Das Zentrieren und der lange Druck auf Shift bleiben am Handkontroller, in der Okularmitte.

### Welches Sternpaar

Zwei helle Sterne wählen, gut über dem Horizont, mindestens etwa vierzig Grad voneinander entfernt, nicht beide nahe am Pol und nicht beide nahe am Horizont. Ein Stern im Osten und einer im Westen, mit unterschiedlicher Deklination, geben ein stabiles Modell. Einen falsch bestimmten Stern nicht bestätigen: ein einziger Katalogfehler überträgt sich auf den ganzen Himmel.

**Show align. error** zeigt den restlichen Winkelfehler des Modells. Ein Fehler von mehreren Zehnern Bogenminuten ist ein Grund, die Prozedur zu wiederholen, nicht ein Grund, ihn mit Motorspiel auszugleichen.

### Was die Ausrichtung nicht behebt

Sie ersetzt keine falsche Getriebezahl, keine verkehrte Drehrichtung und keine falsche Uhr. Liegt das erste Goto um Grade neben dem Ziel, zuerst die Kapitel 8 bis 10 wiederholen, bevor eine neue Ausrichtung gestartet wird.

---

## 16. Deutsche Montierung

Der Tubus kann auf der einen oder der anderen Seite der Säule stehen. Die Anzeige zeigt die Seite. Ein Goto, das den Meridian über die Grenzen hinaus queren würde, löst ein **Umschw.** aus: der Tubus wechselt die Seite, Rektaszension und Deklination des Objekts bleiben dieselben.

**Goto → Umschw.** verlangt es sofort. Liegt die Ankunftslage außerhalb der Grenzen, zeigt die Anzeige **Umschw.** / **Nicht möglich**.

**Montierungs-Seite** erzwingt Ost oder West, wenn die Montierung diese Information verloren hat (Klemmen gelöst, von Hand bewegt). Danach verlangt das Menü eine Synchronisierung auf ein Ziel. Ohne sie stimmen angezeigte Seite und Himmel nicht überein.

### Meridiangrenzen

**Grenzen → Deutsche Monti.**

- **Meridian O** und **Meridian W**: von −45° bis +45°. Sie erlauben die Nachführung ein Stück über den Meridian hinaus, bevor umgeschwenkt wird, damit eine Belichtung nicht genau am Durchgang abbricht.
- **Unter dem Pol**: maximaler Stundenwinkel, von 9 h bis 12 h. Er hält Tubus oder Gegengewichtsstange vom Stativ fern, wenn ein Objekt unter dem Pol nachgeführt wird.

Diese Werte für die eigene Mechanik setzen (Tubuslänge, Stativhöhe), nicht für den Katalog. Ein abgelehntes Objekt meldet sich als **Ausserh. Grenzen**, **Unter Horizont**, **Nah am Zenit** oder als Meridiangrenze.

---

## 17. Altazimutale Montierung

Die Basis muss waagerecht stehen. Eine Libelle auf dem Sockel gehört zur Aufstellung: die Zweistern-Ausrichtung nimmt einen Rest auf, nicht eine um mehrere Grad schiefe Basis.

Home als Voreinstellung: Tubus waagerecht nach Süden. Eine andere Lage lässt sich mit **Setze Homeposition** festlegen, sobald der Tubus dort steht.

Die Nachführung korrigiert ständig beide Achsen, weil keine Achse parallel zur Erdachse liegt. Zeit und Ort sind notwendig: ohne sie sind Bildfelddrehung und Nachführung falsch, auch wenn der Tubus auf ein Objekt synchronisiert wurde.

**Unter dem Pol** gilt für diesen Montierungstyp nicht. Die nützlichen Grenzen sind Horizont, Zenit und die Achsanschläge.

Nahe am Zenit verlangt eine kleine Bewegung am Himmel eine große Bewegung im Azimut. Die **Zenitgrenze** (nächstes Kapitel) meidet diesen Bereich.

---

## 18. Grenzen

**Montierung → Grenzen**

| Zeile | Bedeutung | Eingabebereich |
|-------|-----------|----------------|
| Horizont | Mindesthöhe eines Goto | −10° bis +20° |
| Zenit | Maximalhöhe | 60° bis 91° |
| Axis | Mechanische Anschläge beider Achsen | in Grad, nach Anzeige der aktuellen Lage |
| Deutsche Monti. | Meridian und unter dem Pol | siehe Kapitel 16 |

**Horizont** auf 0° lehnt alles unter dem Horizont ab. Ein negativer Wert erlaubt ein geringes Unterschreiten, zum Beispiel von einem Standort mit freiem Abfall. Ein positiver Wert hält den Tubus über einer Mauer oder einer Hecke.

**Axis** zeigt zuerst die Lage, dann **Axis1 Min.**, **Axis1 Max.**, **Axis2 Min.**, **Axis2 Max.** Den Tubus von Hand oder langsam mit dem Motor an jeden wirklichen Anschlag fahren, den Winkel lesen und die Grenze um ein oder zwei Grad zurücknehmen, damit die Mechanik nicht anschlägt.

Ein abgelehntes Goto ist kein Defekt. Die Meldung lesen: **Unter Horizont**, **Nah am Zenit**, **Ausserh. Grenzen**, Deklinationsgrenze oder Azimutgrenze.

---

## 19. Getriebeprüfung

**Teleskop Aktion → Getriebe-Check** vergleicht die gespeicherte Getriebezahl mit der wirklichen Bewegung.

Die Firmware verlangt eine Synchronisierung auf einen Stern A, dann ein Goto nach B (große Bewegung vor allem auf Achse 1) und nach C (große Bewegung vor allem auf Achse 2). Bei einer äquatorialen Montierung schlägt sie einen großen Stundenwinkel vor, dann eine große Deklination. Bei einer altazimutalen einen großen Azimut, dann eine große Höhe.

Bei jeder Ankunft:

1. Das Ende des Schwenks abwarten (**Warte auf Slew...**). Ein langes Shift bricht ab.
2. Den Stern neu zentrieren.
3. Kurzer Druck auf Shift zur Bestätigung.

Die Anzeige zeigt den Fehler jedes Schritts in Bogenminuten und die gemessene Getriebezahl. Die Formel lautet: gemessene Zahl = gespeicherte Zahl × befohlene Bewegung / wahre Bewegung. Eine befohlene Bewegung unter 5° wird verworfen: der Maßstab wäre zu schlecht.

Liefern die beiden Achsen widersprüchliche Verhältnisse, meldet die Anzeige **Achsen aehnlich**: die gewählten Sterne haben die Achsen nicht genug getrennt. Mit weiter auseinanderliegenden Zielen neu beginnen.

Die gemessene Zahl nur dann unter **Motor → Getr. Zahl** eintragen, wenn die Drehrichtung schon stimmt und sorgfältig zentriert wurde. Ein schlecht zentrierter Stern wird unmittelbar zu einem Getriebefehl.

---

## 20. Encoder und Push-to

Encoder messen die wirkliche Achslage. Sie dienen dem Push-to (der Tubus wird geschoben, die Anzeige zeigt, wie das Ziel erreicht wird) und können die Motoren nachführen.

**Montierung → Encoders → Aktivieren**, danach Neustart.

Danach:

| Zeile | Aufgabe |
|-------|---------|
| Auto Sync | Automatisches Nachziehen des Motors auf den Encoder |
| Calibration | Einmessung an einem Stern |
| Pulse per deg E1 / E2 | Auflösung des Encoders |
| Drehung E1 / E2 | Richtung des Encoders |
| Deaktivieren | Schaltet die Encoder aus, mit Neustart |

**Pulse per deg** geht von 0,50 bis 3600 Impulse je Grad.

**Auto Sync**: AUS, 60', 30', 15', 8', 4', 2' oder AN. Der Motor wird auf den Encoder gezogen, wenn die Abweichung die Schwelle überschreitet, und nur außerhalb eines Schwenks. **AN** zieht fortlaufend in der feinsten Toleranz, die die Firmware vorsieht. **AUS** lässt die Encoder nur anzeigend.

### Einmessung

1. **Stern**: startet die Einmessung.
2. Den Referenzstern anfahren und zentrieren, wie die Anzeige es verlangt.
3. **beendet** speichert. **Abbruch** verwirft.

Die Richtung **Direkt** / **Rückwärtig** jedes Encoders wird wie bei den Motoren gesetzt: entfernt sich das Push-to, wenn in die angezeigte Richtung geschoben wird, diesen Encoder umkehren.

### Push-to

Sind Encoder aktiv, enthält **Teleskop Aktion** die Zeile **Pushto**. Das Ziel wie bei einem Goto wählen. Die Anzeige wechselt auf die Abstandsseite. Den Tubus von Hand bewegen, bis die Abweichung null ist. Ein kurzer Druck auf Shift bietet dann an, die Motoren auf die Encoder zu synchronisieren.

Ohne Encoder gibt es diesen Eintrag nicht: das Aktionsmenü ist das aus Kapitel 14, ohne Pushto.

---

## 21. Autoguider-Buchse

Die ST-4-Buchse des Steuergeräts nimmt die vier Richtungen eines Autoguiders oder einer Nachführkamera an. Die verwendete Geschwindigkeit ist die **Guiding Geschw.** aus Kapitel 12.

Die Nachführung muss laufen. Eine ST-4-Korrektur legt sich auf die Nachführung; sie ersetzt sie nicht. Der Guiding-Zustand (Impuls, ST-4, Nachzentrieren) ist für angeschlossene Programme sichtbar, und die Bewegung ist im Okular zu sehen.

Die Guiding-Geschwindigkeit so wählen, dass der Stern bei einem Impuls von einer Sekunde deutlich wandert, ohne den ganzen Sensor zu queren. 0,5× Sternzeit ist ein üblicher Anfang. Bei langer Brennweite gegen 0,3× gehen, gegen 0,8×, wenn die Korrekturen keine Wirkung haben.

---

## 22. Fokus

Ist kein Fokusierer verbunden, zeigen Shift + F oder Shift + f **Fokusierer** / **Nicht verbunden**.

**Fokusierer Aktion** (Shift + f): bereits gespeicherte benannte Positionen, **Goto**, **Synchronisierung**, **Park**, **Sperre**. Die Wahl einer benannten Position fährt den Fokusierer dorthin. **Synchronisierung** erklärt die aktuelle Lage zur Referenz. **Park** stellt den Fokusierer auf seine Parkposition.

**Fokusierer Einst.** (Shift + F):

- **Config**: Anzeige, Parkposition, Maximalposition, Hand- und Goto-Geschwindigkeit, Beschleunigungen.
- **Motor**: Auflösung, Drehrichtung, Schritte pro Umdrehung, Mikroschritt (4, 8, 16, 32, 64, 128), Strom.
- **Info Focuser**: Version, Neustart, Werksreset nur des Fokusierers.

Die Sperre des Fokusierers verhindert, dass ein versehentlicher Druck die Schärfe während einer Belichtung verliert.

---

## 23. WLAN, Telefon, Rechner

**Teleskop-Einst. → Wifi**

- **WLAN anschalten** oder **WLAN ausschalten**
- **Passwort anzeigen**
- **Modus wählen**: bis zu drei Stationen, dazu der Zugangspunkt des Handkontrollers
- **IP-Adresse zeigen**
- **zurücksetzen auf Werk** der WLAN-Einstellung des Handkontrollers

Ein- oder Ausschalten des WLAN verlangt einen Neustart.

Im Zugangspunkt tritt Telefon oder Rechner dem Netz des Handkontrollers bei, mit dem angezeigten Passwort. Als Station tritt der Handkontroller dem Router bei: die Adresse ist die, die **IP-Adresse zeigen** nennt, keine feste Adresse.

Die TCP-Brücke hört auf Port **9999** und trägt denselben Dialog wie das USB-Kabel des Steuergeräts. TeenAstro-App, ASCOM-Treiber und Planetariumsprogramme verbinden sich dort.

Die App bietet Übersicht, Planetarium, Goto und Ausrichtung. Der ASCOM-Treiber öffnet TeenAstro für Stellarium, Cartes du Ciel, NINA und andere ASCOM-Clients unter Windows.

Eine Webseite der Schnittstelle nimmt Netze und Passwort entgegen, ohne den Handkontroller. Die Adresse ist die von **IP-Adresse zeigen**.

### SkySafari

Nötig ist SkySafari Plus oder Pro, die Ausgabe, die ein Teleskop steuert. Telefon und Handkontroller müssen im selben Netz sein.

Als Zugangspunkt, wie ausgeliefert: das Netz heißt **TeenAstro**, die Adresse ist **192.168.0.1**, der Port ist **9999**. Das Werkspasswort ist `password`; **Passwort anzeigen** zeigt das tatsächlich gespeicherte, falls es geändert wurde. Als Station hängt das Telefon am Router, und die Adresse ist die von **IP-Adresse zeigen**, weiterhin Port 9999.

In SkySafari: Einstellungen, Teleskop, Konfiguration.

| Einstellung | Wert |
|-------------|------|
| Montierungstyp | Äquatoriales GoTo oder altazimutales GoTo, passend zur Montierung |
| Teleskoptyp | Meade LX-200 Classic |
| Verbindung | WLAN. Nicht der SkyFi-Modus |
| Adresse | 192.168.0.1 als Zugangspunkt, sonst die von **IP-Adresse zeigen** |
| Port | 9999 |

**Set Time & Location**, falls angeboten, sendet die Uhr des Telefons, und den Ort nur, solange die Montierung auf Home oder Park steht. Sind Zeit und Ort am Handkontroller schon richtig, das Kästchen aus lassen: das Telefon würde den Ort überschreiben. Soll dagegen das GPS des Telefons gelten, zuerst parken oder auf Home stehen, dann das Kästchen setzen und verbinden.

Verbinden. Die Karte muss das Fadenkreuz des Teleskops zeigen. Ein Objekt antippen, dann Goto. Der Handkontroller zeigt das Schwenksymbol. Nur ein Goto zur Zeit: das von SkySafari und das vom Handkontroller unterbrechen einander.

Das Zentrieren bleibt an den Tasten, langsam, dann Guiding. Die Synchronisierung kann von SkySafari oder vom Handkontroller kommen, auf dasselbe Objekt. Die Zweistern-Ausrichtung aus Kapitel 15 bleibt am Handkontroller einfacher; SkySafari dient danach der Objektwahl.

Während ein Rechner die Montierung führt, bleibt der Handkontroller benutzbar. Nicht zwei Gotos zugleich starten: das zweite bricht das erste ab.

### Firmware aktualisieren

Das Wiki der Gruppe beschreibt TeenAstroUploader für Windows.

1. Vor einer Aktualisierung die Parameter mit TeenAstroConfig sichern. Ein Versionswechsel kann den Speicher zurücksetzen, wenn der interne Schlüssel sich geändert hat.
2. USB-Kabel nur am Anschluss des Steuergeräts, Montierung eingeschaltet. Beim ersten Mal installiert Windows das Teensy-Gerät.
3. Im Werkzeug die Platine wählen. Der vierte Bildschirm nach dem Einschalten nennt das Modell. Im Zweifel die Beschriftung auf der Platine lesen.
4. Das Senden starten und das Ende des Ladeprogramms abwarten.
5. Für den Handkontroller über WLAN: die Adresse mit **IP-Adresse zeigen** ablesen, im Werkzeug eintragen, über WLAN senden.

Handkontroller und Steuergerät müssen aus derselben Veröffentlichung stammen. Sonst kehrt der Versionsfehler schon beim Start zurück.

**Steuergerät Info → Versionsnr. zeigen** zeigt Name und Datum der Firmware. **Neustart** startet das Gerät neu. **zurücksetzen auf Werk** löscht den Speicher nach der Bestätigung NEIN / JA.

---

## 24. Besuchermodus

**SHC Einst. → Rechte**: **Admin.** oder **Besucher**.

Als Besucher enthält **Teleskop-Einst.** nur noch **Rechte**. Mechanik, Zeit und WLAN sind verborgen. Die Handlungen der Nacht (Goto, Synchronisierung, Nachführung, Geschwindigkeiten) bleiben verfügbar. Das ist der Modus für eine gemeinsam genutzte Montierung oder eine Vorführung.

Die Wahl wird im Handkontroller gespeichert. Zurück zum Administrator: Shift + West, **Rechte**, **Admin.**

---

## 25. Ein Beobachtungsabend

Das Beispiel ist ein Spätsommerabend um 45° nördlicher Breite: Wega und Atair stehen hoch, M13 ist noch da, M31 geht auf. In einer anderen Nacht bleiben die Handgriffe, die Namen wechseln. Die Filter stehen in Kapitel 14, SkySafari in Kapitel 23.

### Eine leere Batterie erkennen

Der Handkontroller hat keine Anzeige dafür. Nichts auf dem Bildschirm sagt «Batterie schwach». Man sieht es am Verhalten, und an der Kontrolle vor der Nacht.

Vor dem Einschalten ist ein Voltmeter an der Batterie die einzige verlässliche Zahl. Die genaue Schwelle hängt von der Chemie (Blei oder Lithium) und von der Platine ab: das Wiki der Gruppe nennt die Spannung der jeweiligen Version. Eine Batterie, die in Ruhe hält und einbricht, sobald ein Motor beschleunigt, ist schon zu schwach.

Während der Sitzung sieht eine nachlassende Batterie so aus, ohne dass ein Kabel bewegt wurde:

- die Anzeige wird blass, flackert, oder der Handkontroller startet von selbst beim Logo neu;
- **Nicht verbunden**, dann Neustart, ohne dass jemand das Kabel berührt hat;
- ein Goto mit Max bleibt stehen, der Motor brummt, der Tubus hält vor dem Objekt an, und dieselbe Bewegung mit Langsam gelingt;
- das WLAN verschwindet und **IP-Adresse zeigen** antwortet bis zum nächsten Start nicht mehr;
- die Nachführung stoppt und die Koordinaten passen nicht mehr zum Himmel, weil das Steuergerät neu gestartet ist und den Faden verloren hat.

Das ist keine leere Batterie: ein schwarzer Bildschirm, den Shift weckt, ist die Ruheabschaltung; **ERROR** / **version** ist Firmware, die nicht zusammenpasst.

Treten die Zeichen während eines Goto auf, die Bewegung anhalten (Shift lang). Nur parken, wenn die Motoren noch antworten. Laden oder tauschen, bevor es weitergeht: verlorene Schritte machen das Zeigen falsch, bis zur nächsten Synchronisierung.

### Inbetriebnahme

1. Tubus auf Park, oder auf Home, wenn noch kein Park festgelegt ist. Anschließen, einschalten, Logo, Versionen und Uhrzeit durchlaufen lassen.
2. Zeit und Ort prüfen (**Zeit & Ort**). Ohne GNSS sind es die Werte, die eingegeben wurden.
3. Fehlt das Nachführsymbol: **Teleskop Aktion → Nachführung → Nachführ. starten**.
4. Bietet das Aktionsmenü nur **Unpark**, ist die Montierung geparkt. Entparken. Das Parksymbol muss verschwinden.

### Auf zwei Sterne ausrichten

Gibt es schon ein Modell und hat sich die Aufstellung nicht bewegt: **Goto → Kataloge → leuchtende Sterne**, Wega wählen, mit Langsam zentrieren, und nur synchronisieren, wenn der Stern im Feld ist. Danach zu den Objekten.

Sonst eine Zweistern-Ausrichtung von Home (Kapitel 15), mit zwei benannten Sternen, hoch und weit auseinander:

1. Erster Stern: Wega, in der Leier. Der Tubus fährt, die Anzeige sagt **Fahre zu**, dann **Neuzentrieren**. Zentrieren. Langer Druck auf Shift: **Stern hinzugefügt**.
2. Zweiter Stern: Atair, im Adler. Er liegt weit von Wega im Stundenwinkel. Dieselbe Zentrierung, derselbe lange Druck.
3. **Speichern**. Das Ausrichtungssymbol bestätigt, dass das Modell gespeichert ist.

Ein zu nahes Paar, Wega und Deneb zum Beispiel, gibt ein schwaches Modell. Kapitel 15 sagt, wie man wählt.

### Erstes Objekt: M13

M13 ist der Kugelsternhaufen im Herkules, im Sucher leicht zu finden, sobald das Modell steht.

1. **Goto → Kataloge → Filters**.
2. **Above Horizon**, dann **Über 30 deg.** M13 soll hoch genug stehen; filtert die Zeile ihn weg, ist das richtig so.
3. **Constellation**, dann **Her**.
4. **Type**, dann **Globular Clstr**.
5. Zurück zu den Katalogen, Messier öffnen. Der Titel wird **!Goto Messier!**: die Filter arbeiten. Bis M13 blättern. F startet das Goto.
6. Bei der Ankunft **Langsam** im Sucher, **Guiding** im Okular. Nur synchronisieren, wenn der Haufen sicher in der Mitte steht. Eine Synchronisierung auf das falsche Objekt verschiebt den ganzen Himmel.

Sagt die Anzeige **No Object**, steht M13 unter 30° oder der Typ passt nicht. **Grundstellung Filters**, dann mit **Über 10 deg.** erneut versuchen.

### Zweites Objekt: M31

M31, die Andromedagalaxie, liegt nicht im Herkules. Bleibt der Sternbildfilter, erscheint sie nicht.

1. **Filters → Constellation → And**.
2. **Type → Galaxy**. Die Höhe bei 30° lassen, wenn Andromeda schon oben ist, sonst auf **Über 10 deg.** senken.
3. Messier, M31, F.
4. Das Feld ist weit: langsame Geschwindigkeit, und der Sucher statt starker Vergrößerung, um sie zu finden.

Für einen Planetarischen Nebel in derselben Himmelsgegend wie Wega: **Constellation → Lyr**, **Type → Planetary Nebula**, dann M57.

Mond, Jupiter oder Saturn gehen über **Goto → Sonnensystem**, ohne Filter. Die Sonne nur mit einem Filter über die volle Öffnung. Die Steuerung prüft das nicht.

### Ende der Nacht

**Goto → Park**. Auf das Parksymbol warten, nicht nur auf den Stillstand der Motoren. Dann die Spannung abschalten. Ein abgebrochenes Parken lässt die Position beim nächsten Einschalten falsch.

Mussten die Klemmen in der Nacht gelöst werden, mindestens neu synchronisieren, und neu ausrichten, wenn der Tubus weit bewegt wurde, ohne dass die Encoder mitgegangen sind.

---

## 26. Wenn etwas nicht stimmt

| Was zu sehen ist | Wohin schauen |
|------------------|---------------|
| **FEHLER** / **version** | Handkontroller und Steuergerät aus verschiedenen Veröffentlichungen |
| **Nicht verbunden**, dann Neustart | Kabel des Handkontrollers, Batterie, die unter Last einbricht, oder Steuergerät aus |
| Die Anzeige wird blass, der Handkontroller startet beim Logo neu, ein Goto mit Max bleibt stehen | Batterie leer oder unter Last zu schwach. Der Handkontroller zeigt keine Spannung. Kapitel 25 |
| Schwarzer Bildschirm, Shift weckt ihn | Ruheabschaltung, keine leere Batterie |
| Die Tasten fahren verkehrt | Kapitel 8, Drehrichtung |
| Goto liegt immer daneben, um den Faktor zwei oder mehr | Getriebezahl oder Schrittzahl. Getriebe-Check ausführen |
| Goto stimmt nahe dem Sync-Stern und wird weiter weg schlechter | Keine Ausrichtung, oder der falsche Stern. Falsche Zeit oder falscher Längengrad |
| **Unter Horizont** oder **Ausserh. Grenzen** bei einem sichtbaren Objekt | Horizontgrenze, falscher Ort oder falsche Montierungsseite |
| **Nah am Zenit** | Zenitgrenze, oder das Objekt steht für die Mechanik wirklich zu hoch |
| Der Motor setzt beim Goto aus | Hoher Strom zu niedrig, Silent, Beschleunigung zu hart, Maximalgeschwindigkeit zu hoch |
| Der Motor wird im Stillstand warm | Niedriger Strom zu hoch |
| Der Stern springt bei jeder Tastenumkehr | Spiel zu groß, oder Spiel null, obwohl die Mechanik welches hat |
| Die Ausrichtung scheitert sofort | Die Montierung stand nicht auf Home, oder die erste Synchronisierung wurde abgelehnt |
| **Umschw.** / **Nicht möglich** | Die Ankunftslage würde eine Meridian- oder Achsgrenze überschreiten |
| Push-to entfernt sich vom Ziel | Encoder-Richtung verkehrt, oder falsche Impulse je Grad |
| WLAN zeigt keine Adresse | WLAN aus, falscher Modus, oder der Neustart steht noch aus |

Vor dem Löschen des Speichers: die Parameter notieren oder die Sicherung von TeenAstroConfig wieder öffnen. Der Werksreset ist der letzte Schritt, nicht der erste.

---

## Anhang A. Menübaum

Zeilen in Klammern erscheinen nur im genannten Fall.

**Shift + Ost — Teleskop Aktion**

- (wenn geparkt) Unpark
- Goto — Kataloge, Sonnensystem, Koordinaten, Benutzer definiert, Home, Park, Umschw.
- (Encoder) Pushto — Kataloge, Sonnensystem, Koordinaten, Benutzer definiert
- Synchronisierung — wie Goto, ohne Umschw.
- Ausrichten — 2 Sterne, (äquatorial) 2 Sterne Mech., Computer Ausrichtung, danach Speichern, Löschen, Show align. error
- Getriebe-Check
- Nachführung
- Montierungs-Seite
- Speichern RADEC
- Sperre
- Spiral

**Shift + Nord — Geschw. setzen**: Guiding, Langsam, Mittel, Schnell, Max

**Shift + West — Teleskop-Einst.** (Administrator)

- Handkontroller — Rechte, Anzeige, Geschw. Knopf, Ergonomie, Reset
- Zeit & Ort — Zeit (Uhr, Zeitzone, Datum, GNSS Zeit), Ort, Sync mit GNSS
- Park und Home — Setze Parkposition, Setze Homeposition, Reset HomePosition
- Montierung — Montierung, Montierungs-Art, Motoren, Encoders, Grenzen, Refraktion, Strichplatte
- Steuergerät Info — Versionsnr. zeigen, Neustart, zurücksetzen auf Werk
- Wifi

**Motoren → Motor 1 oder 2**: Zeige Einst., Drehung, Getr. Zahl, Schr. pro Rot., Mikroschritt, Getr. Spiel, Spiel Geschw., niedrige Str., hohe Str., Silent

---

## Anhang B. Griechische Buchstaben

Die Sternkataloge verwenden Bayer-Buchstaben. Die deutsche Firmware nennt den Katalog «leuchtende Sterne».

| Buchstabe | Name | Buchstabe | Name |
|-----------|------|-----------|------|
| α | Alpha | ν | Ny |
| β | Beta | ξ | Xi |
| γ | Gamma | ο | Omikron |
| δ | Delta | π | Pi |
| ε | Epsilon | ρ | Rho |
| ζ | Zeta | σ | Sigma |
| η | Eta | τ | Tau |
| θ | Theta | υ | Ypsilon |
| ι | Iota | φ | Phi |
| κ | Kappa | χ | Chi |
| λ | Lambda | ψ | Psi |
| μ | My | ω | Omega |

Einige häufige Eigennamen in diesem Katalog: Sirius, Canopus, Arktur, Wega, Capella, Rigel, Prokyon, Beteigeuze, Atair, Aldebaran, Spica, Antares, Pollux, Deneb, Regulus.

Für die Ausrichtung ist ein benannter, hoch stehender Stern mehr wert als ein schwacher Stern, bei dem man unsicher ist.

---

## Anhang C. Die Getriebezahl rechnen

Getriebezahl = (Zähne des Rades) × (Verhältnis jeder vorgeschalteten Untersetzung oder Riemenscheibe).

| Aufbau | Rechnung | Einzutragender Wert |
|--------|----------|---------------------|
| Schnecke, Rad 360, Motor direkt | 360 × 1 | 360 |
| Rad 180, Riemenscheibe 16/8 | 180 × 2 | 360 |
| Rad 144, Getriebe 10:1 | 144 × 10 | 1440 |
| Zahnkranz 360, Stufen 3:1 und 2:1 | 360 × 3 × 2 | 2160 |

Die Schritte pro Umdrehung sind die des nackten Motors (200 bei 1,8°, 400 bei 0,9°), nicht das Produkt mit den Mikroschritten. Die Mikroschritte werden getrennt eingestellt.

Ungefähre Auflösung am Himmel, in Bogensekunden je Mikroschritt:

360 × 3600 / (Getriebezahl × Schritte pro Umdrehung × Mikroschritte)

Beispiel: Getriebezahl 360, Motor 200 Schritte, 16 Mikroschritte → 11,25" je Mikroschritt. Das ist die Größenordnung der kleinsten Bewegung, vor der Interpolation des Treibers. Es ist nicht die Treffgenauigkeit, die von Spiel, Durchbiegung und Ausrichtung abhängt.

---

## Anhang D. Weiterführendes

- Gruppe und Wiki: [https://groups.io/g/TeenAstro/wiki/home](https://groups.io/g/TeenAstro/wiki/home). Erste Inbetriebnahme, bebilderte Menüs, Platinen, das Sicherungswerkzeug und das Flashen stehen dort.
- Technische Dokumentation im Depot: [docs/README.md](../README.md) (Aufbau, Nachführung, Protokoll). Sie richtet sich an jemanden, der die Software ändert, nicht an die Führung der Nacht.
- FS2-Anleitung, für die ursprüngliche Denkweise: [anleit_f.pdf](https://www.astro-electronic.de/anleit_f.pdf), Astro-Electronic, Michael Koch. Die deutsche Ausgabe liegt auf derselben Website.

Platinen, Versorgungsspannungen und Motorkabel sind von Version zu Version nicht dieselben. Für die Verdrahtung von der Wiki-Seite der eigenen Karte ausgehen, und für den Betrieb hierher zurückkommen.
