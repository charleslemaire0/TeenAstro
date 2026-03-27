# TeenAstro Release Notes since version 1.5

## English

Since version 1.5, TeenAstro has received a major functional upgrade for both visual observers and imaging users. The focus was not only on bug fixes, but on a more modern control architecture with higher precision, less configuration effort, and a better mobile workflow.

### ASCOM 7.1: key changes and benefits

- **ASCOM V7.1 generation driver** with a modern local-server architecture and improved compatibility with current 64-bit astronomy software.
- **ASCOM Hub / Device Hub is no longer required** for normal Telescope + Focuser use. The TeenAstro driver now handles shared access directly.
- **Telescope and Focuser drivers are fused at connection level**: both interfaces use the same COM/IP session and keep mount/focuser state synchronized.
- **Binary protocol for highest accuracy and speed**: bulk commands (`:GXAS#`, `:GXCS#`) and high-precision rate paths (including float64/double handling) reduce latency and rounding drift.
- **Practical result for users**: faster state refresh, better coordinate consistency, more reliable guiding/tracking behavior, and fewer client disconnect/reconnect issues in real sessions.

### Android app: expanded user workflow

The Android app has evolved from a basic controller into a full observing companion:

- **Connection and live dashboard**: connect over WiFi/TCP, monitor mount status in real time (RA/Dec, Alt/Az, tracking, park/home state, timers).
- **Goto and object selection**: choose targets from catalogs (Messier, NGC, stars and more), or use coordinate entry workflows.
- **Planetarium view**: interactive sky map with stars, planets, constellation lines, and object-centered navigation.
- **Alignment workflow**: multi-star alignment flow with tighter integration to mount state and clearer progress handling.
- **Tracking and motion control**: manual slew control, tracking mode controls, and safer operation during active sessions.
- **Coordinate handling improvements**: better J2000/JNow consistency for display and operations, reducing confusion between chart and mount behavior.
- **Night usability**: night-view support and streamlined navigation to reduce friction at the telescope.

### Overall user impact since 1.5

- More reliable GOTO, tracking, and alignment.
- More stable SHC behavior during slews and alignment phases.
- Stronger ASCOM interoperability with modern client software.
- Better day-to-day experience on Android and Windows app workflows.
- More robust WiFi/Web behavior in field conditions.

### Downloads

- ASCOM Driver Setup (Windows EXE): [TeenAstro Setup 1.6.exe](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/driver/TeenAstro%20Setup%201.6.exe)
- Firmware Uploader (Windows MSI): [TeenAstroUploader.msi](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/Firmware/TeenAstroUploader.msi)
- TeenAstro App (Windows EXE): [teenastro_app.exe](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/App/Windows/teenastro_app.exe)
- TeenAstro App (Android APK): [teenastro_app-release.apk](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/App/teenastro_app-release.apk)

## Francais

Depuis la version 1.5, TeenAstro a recu une evolution majeure pour l'observation visuelle et l'imagerie. L'objectif etait d'ameliorer la precision, la stabilite et le confort d'utilisation, avec une architecture plus moderne et plus simple a exploiter.

### ASCOM 7.1: evolutions principales et benefices

- **Pilote de generation ASCOM V7.1** avec architecture locale moderne et meilleure compatibilite avec les logiciels astronomiques 64 bits.
- **ASCOM Hub / Device Hub n'est plus necessaire** pour un usage normal Telescope + Focuser.
- **Pilotes Telescope et Focuser fusionnes au niveau connexion**: une seule session COM/IP partagee, avec etats monture/focuser coherents.
- **Protocole binaire pour une precision maximale**: commandes bulk (`:GXAS#`, `:GXCS#`) et gestion haute precision des vitesses (float64/double) pour limiter latence et erreurs d'arrondi.
- **Impact concret**: rafraichissement d'etat plus rapide, coordonnees plus coherentes, suivi/guidage plus fiable, moins de deconnexions cote client.

### Application Android: fonctions et usage terrain

L'application Android est devenue un vrai compagnon d'observation:

- **Connexion et tableau de bord temps reel**: liaison WiFi/TCP, affichage direct RA/Dec, Alt/Az, suivi, etat park/home, temporisations.
- **Goto et choix des objets**: selection via catalogues (Messier, NGC, etoiles, etc.) ou saisie de coordonnees.
- **Vue planetarium interactive**: carte du ciel avec etoiles, planetes, lignes de constellations et centrage sur objets.
- **Alignement assiste**: workflow multi-etoiles mieux integre a l'etat monture.
- **Controle de mouvement et suivi**: commandes de deplacement manuel et parametres de suivi adaptes aux sessions reelles.
- **Amelioration J2000/JNow**: affichage/operation plus coherents entre carte, objet et monture.
- **Confort nocturne**: mode nuit et navigation simplifiee pour limiter les manipulations sur le terrain.

### Impact utilisateur global depuis 1.5

- GOTO, suivi et alignement plus fiables.
- Meilleure stabilite SHC pendant slews et alignements.
- Interoperabilite ASCOM nettement renforcee.
- Experience quotidienne amelioree sur Android et Windows.
- Comportement WiFi/Web plus robuste en conditions reelles.

### Telechargements

- Installation pilote ASCOM (EXE Windows): [TeenAstro Setup 1.6.exe](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/driver/TeenAstro%20Setup%201.6.exe)
- Uploader firmware (MSI Windows): [TeenAstroUploader.msi](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/Firmware/TeenAstroUploader.msi)
- Application TeenAstro (EXE Windows): [teenastro_app.exe](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/App/Windows/teenastro_app.exe)
- Application TeenAstro (APK Android): [teenastro_app-release.apk](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/App/teenastro_app-release.apk)

## Deutsch

Seit Version 1.5 hat TeenAstro einen deutlichen Funktionssprung gemacht. Neben Stabilitaetsfixes wurde die Architektur modernisiert, damit Beobachtung und Imaging im Alltag praeziser, schneller und einfacher laufen.

### ASCOM 7.1: wichtige Neuerungen und Nutzen

- **ASCOM V7.1 Treibergeneration** mit moderner Local-Server-Architektur und besserer Kompatibilitaet zu aktueller 64-Bit-Astrosoftware.
- **ASCOM Hub / Device Hub wird nicht mehr benoetigt** fuer den normalen Betrieb von Telescope und Focuser.
- **Telescope- und Focuser-Treiber sind auf Verbindungsebene zusammengefuehrt**: eine gemeinsame COM/IP-Verbindung, synchroner Status.
- **Binaerprotokoll fuer hoechste Genauigkeit und Geschwindigkeit**: Bulk-Kommandos (`:GXAS#`, `:GXCS#`) plus hochpraezise Ratenverarbeitung (float64/double) reduzieren Latenz und Rundungsfehler.
- **Konkreter Anwendernutzen**: schnellere Statusupdates, konsistentere Koordinaten, stabileres Guiding/Tracking und weniger Verbindungsprobleme mit Clients.

### Android-App: deutlich mehr Funktionen in der Praxis

Die Android-App ist inzwischen ein vollwertiger Beobachtungs-Controller:

- **Verbindung und Live-Dashboard**: WiFi/TCP-Verbindung mit Echtzeitstatus (RA/Dec, Alt/Az, Tracking, Park/Home, Zeitdaten).
- **Goto und Zielauswahl**: Objekte aus Katalogen (Messier, NGC, Sterne usw.) oder Koordinateneingabe.
- **Interaktives Planetarium**: Himmelskarte mit Sternen, Planeten, Konstellationslinien und Objektzentrierung.
- **Alignment-Workflow**: Multi-Star-Alignment mit besserem Zusammenspiel von Bedienung und Mount-Status.
- **Bewegungs- und Trackingsteuerung**: manuelles Slew, Tracking-Funktionen und robustere Session-Bedienung.
- **Verbesserte J2000/JNow-Logik**: klarere und konsistentere Darstellung zwischen Karte, Objektinfos und Mount.
- **Nachtbetrieb**: Night-View und optimierte Navigation fuer den praktischen Einsatz am Teleskop.

### Gesamtwirkung fuer Nutzer seit 1.5

- Zuverlaessigeres GOTO, Tracking und Alignment.
- Stabileres SHC-Verhalten waehrend Slew- und Alignment-Phasen.
- Deutlich verbesserte ASCOM-Interoperabilitaet.
- Bessere taegliche Nutzung auf Android und Windows.
- Robusteres WiFi/Web-Verhalten im Feldeinsatz.

### Downloads

- ASCOM-Treiber Setup (Windows EXE): [TeenAstro Setup 1.6.exe](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/driver/TeenAstro%20Setup%201.6.exe)
- Firmware Uploader (Windows MSI): [TeenAstroUploader.msi](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/Firmware/TeenAstroUploader.msi)
- TeenAstro App (Windows EXE): [teenastro_app.exe](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/App/Windows/teenastro_app.exe)
- TeenAstro App (Android APK): [teenastro_app-release.apk](https://github.com/charleslemaire0/TeenAstro/blob/main/Released%20data/App/teenastro_app-release.apk)
