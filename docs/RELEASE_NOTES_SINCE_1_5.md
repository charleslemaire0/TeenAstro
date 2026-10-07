# TeenAstro Release Notes since version 1.5

## English

Since version 1.5, TeenAstro has received a major functional upgrade for both visual observers and imaging users. The focus was not only on bug fixes, but on a more modern control architecture with higher precision, less configuration effort, and a better mobile workflow.

### Latest firmware and ASCOM driver updates

**Firmware**

- Stopping a MoveAxis command now slows down with the configured acceleration. The rate is no longer cut off at once, and tracking on the other axis continues.
- The hand controller guides 2-star, 4-star, and 3+3-star alignment. Four stars on one pier side measure NP. 3+3 measures CH and NP only when each pier side has three stars. Reported terms are Wallace's CH, NP, ID and ME.
- **Mount error**, in the Mount menu above Refraction, holds a known CH and NP during a two-star alignment and during a plate-solve sync, so those terms stay out of the pole estimate.
- The firmware uploader **Auto** button on the telescope and focuser tabs detects the connected board and flashes the matching firmware.

**ASCOM driver**

- MoveAxis again shows the same speed choices as driver 1.5, including 0.25°/s and 0.5°/s. The maximum is the mount's sidereal-rate multiple, and the command sent to the mount uses that same unit.
- Rates, coordinates, and port numbers use a fixed decimal point, so a French or German Windows locale does not change the commands.

### ASCOM 7.1: key changes and benefits

- **ASCOM V7.1 generation driver** with a modern local-server architecture and improved compatibility with current 64-bit astronomy software.
- **ASCOM Hub / Device Hub is no longer required** for normal Telescope + Focuser use. The TeenAstro driver now handles shared access directly.
- **Telescope and Focuser drivers are fused at connection level**: both interfaces use the same COM/IP session and keep mount/focuser state synchronized.
- **Binary protocol for highest accuracy and speed**: bulk commands (`:GXAS#`, `:GXCS#`) and high-precision rate paths (including float64/double handling) reduce latency and rounding drift.
- **GXAS byte 100** now reports the **kind of goto in progress** (EQ, Alt-Az, or meridian flip) in bits 5–7, so apps and the SHC can show the correct slewing context without guessing from mount type alone.
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

- ASCOM Driver Setup (Windows EXE): [TeenAstro Setup 1.6.exe](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/driver/TeenAstro%20Setup%201.6.exe)
- Firmware Uploader (Windows MSI): [TeenAstroUploader.msi](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/Firmware/TeenAstroUploader.msi)
- TeenAstro App (Windows EXE): [teenastro_app.exe](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/App/Windows/teenastro_app.exe)
- TeenAstro App (Android APK): [teenastro_app-release.apk](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/App/teenastro_app-release.apk)

## Francais

Depuis la version 1.5, TeenAstro a recu une evolution majeure pour l'observation visuelle et l'imagerie. L'objectif etait d'ameliorer la precision, la stabilite et le confort d'utilisation, avec une architecture plus moderne et plus simple a exploiter.

### Dernieres mises a jour du firmware et du pilote ASCOM

**Firmware**

- L'arret d'un MoveAxis ralentit maintenant avec l'acceleration configuree. La vitesse n'est plus coupee d'un coup, et le suivi de l'autre axe continue.
- La raquette guide l'alignement 2 etoiles, 4 etoiles et 3+3 etoiles. Quatre etoiles sur un cote de pilier mesurent NP. Le 3+3 mesure CH et NP seulement quand chaque cote a trois etoiles. Les nombres affiches sont CH, NP, ID et ME.
- **Erreur de monture**, dans le menu Mount au-dessus de la refraction, conserve un CH et un NP deja connus pendant un alignement a deux etoiles et pendant une synchro de plate-solve, pour que ces termes ne rentrent pas dans l'estimation du pole.
- Le bouton **Auto** de l'uploader, sur les onglets telescope et focuser, detecte la carte connectee et flashe le firmware correspondant.

**Pilote ASCOM**

- MoveAxis affiche a nouveau les memes vitesses que le pilote 1.5, dont 0,25°/s et 0,5°/s. Le maximum est le multiple de la vitesse siderale de la monture, et la commande envoyee utilise la meme unite.
- Les vitesses, les coordonnees et les ports utilisent un point decimal fixe, pour qu'un Windows francais ou allemand ne modifie pas les commandes.

### ASCOM 7.1: evolutions principales et benefices

- **Pilote de generation ASCOM V7.1** avec architecture locale moderne et meilleure compatibilite avec les logiciels astronomiques 64 bits.
- **ASCOM Hub / Device Hub n'est plus necessaire** pour un usage normal Telescope + Focuser.
- **Pilotes Telescope et Focuser fusionnes au niveau connexion**: une seule session COM/IP partagee, avec etats monture/focuser coherents.
- **Protocole binaire pour une precision maximale**: commandes bulk (`:GXAS#`, `:GXCS#`) et gestion haute precision des vitesses (float64/double) pour limiter latence et erreurs d'arrondi.
- **Octet 100 de GXAS**: les bits 5–7 indiquent le **type de goto en cours** (EQ, Alt-Az ou retournement meridien), pour que l'application et la SHC affichent le bon contexte sans deduire uniquement du type de monture.
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

- Installation pilote ASCOM (EXE Windows): [TeenAstro Setup 1.6.exe](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/driver/TeenAstro%20Setup%201.6.exe)
- Uploader firmware (MSI Windows): [TeenAstroUploader.msi](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/Firmware/TeenAstroUploader.msi)
- Application TeenAstro (EXE Windows): [teenastro_app.exe](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/App/Windows/teenastro_app.exe)
- Application TeenAstro (APK Android): [teenastro_app-release.apk](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/App/teenastro_app-release.apk)

## Deutsch

Seit Version 1.5 hat TeenAstro einen deutlichen Funktionssprung gemacht. Neben Stabilitaetsfixes wurde die Architektur modernisiert, damit Beobachtung und Imaging im Alltag praeziser, schneller und einfacher laufen.

### Neueste Firmware- und ASCOM-Treiber-Updates

**Firmware**

- Das Stoppen eines MoveAxis bremst jetzt mit der eingestellten Beschleunigung. Die Geschwindigkeit wird nicht mehr sofort abgeschnitten, und das Tracking der anderen Achse laeuft weiter.
- Der Handcontroller fuehrt die Ausrichtung mit 2 Sternen, 4 Sternen und 3+3 Sternen. Vier Sterne auf einer Pier-Seite messen NP. 3+3 misst CH und NP nur, wenn jede Pier-Seite drei Sterne hat. Die angezeigten Zahlen sind CH, NP, ID und ME.
- **Mount error**, im Mount-Menue ueber der Refraktion, haelt ein bekanntes CH und NP bei einer Zwei-Stern-Ausrichtung und bei einem Plate-Solve-Sync fest, damit diese Terme nicht in die Polschaetzung eingehen.
- Die Schaltflaeche **Auto** im Firmware-Uploader, auf den Registerkarten Teleskop und Focuser, erkennt die angeschlossene Platine und schreibt die passende Firmware.

**ASCOM-Treiber**

- MoveAxis zeigt wieder dieselben Geschwindigkeiten wie Treiber 1.5, einschliesslich 0,25°/s und 0,5°/s. Das Maximum ist das siderische Vielfache der Montierung, und der Befehl an die Montierung verwendet dieselbe Einheit.
- Raten, Koordinaten und Ports verwenden einen festen Dezimalpunkt, damit ein franzoesisches oder deutsches Windows die Befehle nicht veraendert.

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

- ASCOM-Treiber Setup (Windows EXE): [TeenAstro Setup 1.6.exe](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/driver/TeenAstro%20Setup%201.6.exe)
- Firmware Uploader (Windows MSI): [TeenAstroUploader.msi](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/Firmware/TeenAstroUploader.msi)
- TeenAstro App (Windows EXE): [teenastro_app.exe](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/App/Windows/teenastro_app.exe)
- TeenAstro App (Android APK): [teenastro_app-release.apk](https://github.com/charleslemaire0/TeenAstro/blob/Release_1.6/Released%20data/App/teenastro_app-release.apk)
