# Manuel d'utilisation du système TeenAstro

Autres langues : [English](Manuel_utilisateur_en.md) · [Deutsch](Manuel_utilisateur_de.md)

Manuel de terrain pour l'utilisateur de la monture. Il décrit la raquette, l'unité principale, le suivi, le pointage, l'alignement et les réglages mécaniques tels qu'ils existent dans le firmware actuel.

Il s'inspire de la démarche du manuel FS2 d'Astro-Electronic (connexion, menus, changement d'une valeur, sens de rotation, vitesses, goto, monture allemande, codeurs, mode visiteur), et il s'appuie sur les menus de la raquette, sur l'unité principale, et sur le wiki du groupe [TeenAstro](https://groups.io/g/TeenAstro/wiki/home).

Vous n'avez pas besoin de tout lire avant la première nuit. Pour démarrer, les chapitres 3, 5, 7 et 8 suffisent. Une soirée déjà réglée se conduit avec le chapitre 25. Le reste sert au réglage de la mécanique, à l'alignement fin et au dépannage.

Les noms de menus cités entre guillemets sont ceux du firmware français de la raquette. Quelques libellés restent en anglais dans cette version : `Silent`, `J2000`, `JNow`, `Alt Az`, `Show align. error`, `Wifi`.

---

## 1. De quoi est fait le système

TeenAstro est un contrôleur de monture équatoriale ou altazimutale. Il a été conçu par un utilisateur du FS2 qui voulait un successeur ouvert : mêmes gestes de nuit (raquette, suivi, goto, synchro), avec des moteurs pas à pas, une correction d'alignement à deux étoiles, et une liaison vers un ordinateur.

| Élément | Rôle |
|---------|------|
| Unité principale | Calcule le ciel, commande les moteurs, mémorise la monture, l'heure et le lieu |
| Raquette (SHC) | Écran, sept boutons, catalogues, réglages |
| Interface Wi-Fi | Pont entre l'unité principale et un téléphone ou un ordinateur |
| Mise au point | Moteur de focalisation, en option |
| Application TeenAstro | Tableau de bord, planétarium, pointage |
| Pilote ASCOM | Stellarium, Cartes du Ciel, et les autres logiciels compatibles ASCOM |

L'unité principale est le seul endroit où la position de la monture est tenue. La raquette, le Wi-Fi et l'ordinateur lui envoient des ordres et affichent ce qu'elle répond.

Deux jeux de paramètres de monture peuvent cohabiter (monture 0 et monture 1). On change de jeu dans le menu Monture. Un redémarrage est alors demandé.

---

## 2. Précautions

- Ne branchez pas et ne débranchez pas les câbles moteurs pendant que l'ensemble est sous tension. Un moteur pas à pas déconnecté sous tension peut endommager le driver.
- Avant de modifier le type de monture, la réduction, le nombre de pas ou le sens de rotation, notez les valeurs affichées par **Afficher Paramètres**, ou sauvegardez-les avec TeenAstroConfig. Un mauvais jeu de paramètres transforme une monture qui fonctionne en monture qui ne pointe plus.
- La position **Maison** au moment de la mise sous tension est la référence mécanique. Si le tube n'est pas à cette position, les coordonnées affichées sont fausses jusqu'à une synchro ou un nouvel alignement.
- Le mode **Visiteur** cache les réglages. Repassez en **Administrateur** avant une séance de configuration.
- Une réinitialisation usine efface l'heure, le lieu, la mécanique et l'alignement. Elle demande une confirmation.

Les tensions, les connecteurs et le schéma dépendent de la carte (unité classique, Mini, Tiny). Le câblage propre à chaque carte est décrit dans le wiki du groupe, pas dans ce manuel.

---

## 3. La raquette

Sept boutons : **Shift**, **Nord**, **Sud**, **Est**, **Ouest**, **F** et **f**.

Hors menu, Nord, Sud, Est et Ouest déplacent le tube. La vitesse en cours est rappelée par une icône en haut de l'écran.

| Geste | Effet |
|-------|--------|
| Appui court sur Shift | Page suivante de l'écran |
| Shift maintenu + Est | **Action Télescope** |
| Shift maintenu + Nord | **Réglage Vitesse** |
| Shift maintenu + Sud | **Écran** (extinction, contraste) |
| Shift maintenu + Ouest | **Param. Télescope** |
| Shift maintenu + F | **Param. Focuser** |
| Shift maintenu + f | **Action M.A.P.** |

Dans un menu :

| Bouton | Effet |
|--------|--------|
| Nord / Sud | Monter ou descendre dans la liste |
| Ouest | Entrer dans la ligne, ou augmenter une valeur |
| Est | Revenir en arrière, ou diminuer une valeur |
| F | Valider |
| f ou Shift | Quitter le menu |

**Ergonomie** permet de retourner la raquette pour un gaucher : l'écran pivote de 180° et les boutons Nord/Sud ainsi qu'Est/Ouest sont échangés. Le changement est pris en compte au redémarrage.

**Vitesse Boutons** (Lente, Moyenne, Rapide) règle la répétition des touches, pas la vitesse de la monture.

L'écran s'assombrit puis s'éteint après un délai. **Param. Raquette → Écran** règle la veille, le sommeil profond et le sous-modèle d'OLED. Un appui sur Shift rallume l'écran. **Shift + Sud** permet aussi de l'éteindre tout de suite ou d'en changer le contraste (Min, Faible, Haut, Max).

---

## 4. Lire l'écran

Les photos viennent de l'émulateur de la raquette, logiciel français 1.6.7. L'écran réel fait 128×64 points : ici il est agrandi quatre fois. Dans une version anglaise ou allemande, la disposition est la même et seuls les mots changent.

### Au démarrage

La raquette affiche d'abord le logo, puis les versions, la carte et les drivers, le focuser s'il répond, et enfin l'heure si aucun GNSS n'est connecté.

![Logo TeenAstro](images/screens/boot_logo.png)

![Version de la raquette](images/screens/boot_shc.png)

![Version de l'unité principale](images/screens/boot_main.png)

![Carte et drivers](images/screens/boot_pcb.png)

![Version du focuser](images/screens/boot_focuser.png)

![Heure et date au démarrage](images/screens/boot_clock.png)

Si la raquette affiche **ERREUR** et **version**, son logiciel et celui de l'unité principale ne correspondent pas. Il faut reflasher les deux avec la même publication.

Si elle affiche **pas Connecté**, le lien série avec l'unité principale est perdu. La raquette redémarre. Vérifiez le câble de la raquette avant de chercher plus loin.

### Les pages

Un appui court sur Shift passe à la page suivante. Le fichier `libraries/TeenAstoCustomizations/TeenAstoCustomizations.h` décide lesquelles existent. Telles que livrées, quatre pages sont actives : ascension droite et déclinaison, azimut et hauteur, heure, focuser. Les autres s'obtiennent en retirant le commentaire de la ligne correspondante (`HA_PAGE`, `PUSH_PAGE`, `AXIS_STEP_PAGE`, `AXIS_DEG_PAGE`). Pendant un alignement, l'écran d'alignement remplace la page en cours.

À la maison, sur une monture équatoriale, la déclinaison est au pôle et l'icône maison est allumée.

![Ascension droite et déclinaison](images/screens/radec.png)

Page par défaut. **RA** est l'ascension droite, **Dec** la déclinaison. En haut à gauche : Wi-Fi non relié, puis la vitesse manuelle (ici deux barres, vitesse lente). En haut à droite : la maison.

![Angle horaire](images/screens/hadec.png)

Page optionnelle. **HA** est l'angle horaire, **Dec** la déclinaison. Utile pour voir de quel côté du méridien se trouve l'objet.

![Azimut et hauteur](images/screens/altaz.png)

Page par défaut. **Az.** est l'azimut, **Alt.** la hauteur. À la maison équatoriale le tube pointe le pôle, donc la hauteur dépend de la latitude du site.

![Page push-to](images/screens/push.png)

Page optionnelle. Sans codeurs, elle reste vide : seul le bandeau est dessiné. Avec des codeurs actifs, elle affiche la distance qui reste sur chaque axe pour rejoindre la cible.

![Heure](images/screens/time.png)

Page par défaut. **UTC** est le temps universel, **LST** le temps sidéral local.

![Axes en pas](images/screens/axis_steps.png)

Page optionnelle. Les deux nombres sont les compteurs de pas de l'axe 1 puis de l'axe 2. Ils servent à vérifier qu'un moteur tourne, pas à lire le ciel.

![Axes en degrés](images/screens/axis_deg.png)

Page optionnelle. Les deux premières lignes sont l'angle moteur des axes 1 et 2. Les deux suivantes sont les mêmes angles après correction (jeu, modèle). Sans codeurs, les quatre lignes sont libellées Ax1 et Ax2.

![Focuser](images/screens/focuser.png)

Page par défaut si un focuser répond. **F Position** est la position du foyer, **F Speed** sa vitesse. S'il ne répond pas, la page affiche **Focuser** puis **pas Connecté**.

### Les menus

On entre dans un menu en maintenant Shift, puis en appuyant sur une direction. Shift ou **f** quitte. Est recule. Ouest ou **F** entre dans la ligne en surbrillance. Nord et Sud déplacent la surbrillance.

![Action Télescope, Shift et Est](images/screens/menu_action.png)

**Goto**, **Synchro** et **Aligner** sont les premières lignes. La liste continue avec la vérification des engrenages, le suivi, le côté de pilier, l'enregistrement des coordonnées, le verrou et la spirale. La monture parquée ne propose que le déparc.

![Réglage Vitesse, Shift et Nord](images/screens/menu_speed.png)

La ligne en surbrillance est la vitesse en cours. Les cinq crans sont Guidage, Lente, Moyenne, Rapide et Max. Le nombre de barres à gauche de l'écran principal suit ce choix : une barre pour le guidage, puis deux, trois, quatre, et cinq pour Max.

![Écran, Shift et Sud](images/screens/menu_display.png)

**Éteindre** coupe l'affichage tout de suite. **Contraste** propose Min, Faible, Haut et Max. La police de l'écran n'a pas tous les accents : Écran peut apparaître sans son accent.

![Param. Télescope, Shift et Ouest](images/screens/menu_settings.png)

En mode administrateur : **Raquette**, **Heure & Site**, **Parc et Maison**, puis Monture, les informations de l'unité principale et le Wi-Fi. En mode visiteur, ce menu ne contient que **Privilèges**.

![Param. Raquette](images/screens/menu_shc.png)

**Privilèges** choisit administrateur ou visiteur. **Écran** règle la veille, le sommeil et le modèle d'OLED. **Vitesse Boutons** règle la répétition des touches (lente, moyenne, rapide), pas la vitesse de la monture. Suivent l'ergonomie et la remise à zéro de la raquette.

### Les icônes

Le bandeau fait une rangée de symboles de 16×16 points. À gauche, le Wi-Fi, puis la vitesse si les moteurs sont actifs, puis la flèche Shift pendant que la touche est tenue. À droite, l'état de la monture. Une seule place est prévue pour le suivi, le pointage, la maison ou le parc : le plus prioritaire gagne. Les erreurs s'ajoutent à côté.

**Wi-Fi.** Le boîtier plein signifie relié, le contour seul signifie que l'interface est active mais sans lien. Les quatre dessins sont les trois profils station (0, 1, 2) et le point d'accès.

| Relié | Non relié | Rôle |
|-------|-----------|------|
| ![](images/icons/wifi_sta0.png) | ![](images/icons/wifi_sta0_nc.png) | Station, profil 0 |
| ![](images/icons/wifi_sta1.png) | ![](images/icons/wifi_sta1_nc.png) | Station, profil 1 |
| ![](images/icons/wifi_sta2.png) | ![](images/icons/wifi_sta2_nc.png) | Station, profil 2 |
| ![](images/icons/wifi_ap.png) | ![](images/icons/wifi_ap_nc.png) | Point d'accès |

**Vitesse manuelle.** Le nombre de barres est le cran choisi dans Réglage Vitesse. L'icône n'apparaît que si les moteurs sont sous tension.

| | Cran |
|---|------|
| ![](images/icons/GUIDINGSP.png) | Guidage |
| ![](images/icons/SLOWSP.png) | Lente |
| ![](images/icons/MEDIUMSP.png) | Moyenne |
| ![](images/icons/FASTSP.png) | Rapide |
| ![](images/icons/MAXSP.png) | Max, vitesse de goto |

**Où est le tube.**

| | Signification |
|---|---------------|
| ![](images/icons/home.png) | À la position maison |
| ![](images/icons/parked.png) | Parqué |
| ![](images/icons/parking.png) | Parcage en cours |
| ![](images/icons/parkingFailed.png) | Le parcage a échoué |
| ![](images/icons/no_tracking.png) | Suivi arrêté (deux barres verticales) |
| ![](images/icons/tracking.png) | Suivi en marche, taux non précisé (triangle) |
| ![](images/icons/tracking_star.png) | Suivi sidéral |
| ![](images/icons/tracking_sun.png) | Suivi solaire |
| ![](images/icons/tracking_moon.png) | Suivi lunaire |
| ![](images/icons/tracking_target.png) | Suivi à la vitesse de l'objet (comète, dérive) |
| ![](images/icons/tracking_ra.png) | Suivi avec correction sur l'ascension droite seule |
| ![](images/icons/tracking_both.png) | Suivi avec correction sur les deux axes |
| ![](images/icons/slewing_eq.png) | Goto équatorial |
| ![](images/icons/slewing_altaz.png) | Goto altazimutal |
| ![](images/icons/slewing_flip.png) | Retournement au méridien |
| ![](images/icons/sleewing.png) | Pointage en cours, sans précision du type |
| ![](images/icons/E.png) | Pilier côté est |
| ![](images/icons/W.png) | Pilier côté ouest |

Le triangle du suivi reçoit par-dessus l'étoile, le soleil, la lune ou la cible, puis le chiffre 1 ou 2 si une correction de suivi est active. Pendant un goto, ce triangle disparaît au profit de l'icône de pointage.

**Alignement, guidage, spirale.**

| | Signification |
|---|---------------|
| ![](images/icons/align1.png) | Alignement, étoile 1 |
| ![](images/icons/align2.png) | Alignement, étoile 2 |
| ![](images/icons/align3.png) | Alignement, étoile 3 ou suivante |
| ![](images/icons/Aligned.png) | Un modèle d'alignement est en mémoire |
| ![](images/icons/Spiral.png) | Spirale de recherche en cours |
| ![](images/icons/guiding_.png) | Cadre d'un guidage par impulsions |
| ![](images/icons/guide_n.png) | Impulsion vers le nord |
| ![](images/icons/guide_s.png) | Impulsion vers le sud |
| ![](images/icons/guide_e.png) | Impulsion vers l'est |
| ![](images/icons/guide_w.png) | Impulsion vers l'ouest |
| ![](images/icons/recenter_base.png) | Recentrage (cible) |
| ![](images/icons/atrate_base.png) | Guidage à la vitesse de suivi (réticule) |

Les flèches de guidage se dessinent par-dessus le cadre, la cible ou le réticule, selon que la monture reçoit une impulsion ST-4, un recentrage, ou un guidage à la vitesse de suivi.

**GNSS, verrou, Shift.** L'icône GNSS n'apparaît qu'à la maison ou au parc.

| | Signification |
|---|---------------|
| ![](images/icons/GNSS.png) | Heure et lieu synchronisés |
| ![](images/icons/GNSST.png) | Heure seule |
| ![](images/icons/GNSSL.png) | Lieu seul |
| ![](images/icons/shift.png) | Shift est tenu |
| ![](images/icons/Lock__.png) | Verrou, sans préciser lequel |
| ![](images/icons/lock_t.png) | Télescope verrouillé |
| ![](images/icons/lock_f.png) | Focuser verrouillé |
| ![](images/icons/lock_both.png) | Télescope et focuser verrouillés |

**Erreurs.** Elles remplacent l'état normal du bandeau droit, ou s'y ajoutent. Le texte en abrégé est celui du dessin.

| | Signification |
|---|---------------|
| ![](images/icons/ErrA1.png) | Limite de l'axe 1 |
| ![](images/icons/ErrA2.png) | Limite de l'axe 2 |
| ![](images/icons/ErrHo.png) | Sous l'horizon |
| ![](images/icons/ErrMe.png) | Limite de méridien |
| ![](images/icons/ErrUp.png) | Sous le pôle |
| ![](images/icons/ErrMf.png) | Défaut moteur |

---

## 5. Comment changer une valeur

Le principe est le même partout, comme sur le FS2.

1. Ouvrez le menu (en général Shift + Ouest pour les réglages, Shift + Est pour les actions de nuit).
2. Descendez avec Sud jusqu'à la ligne, puis validez avec Ouest ou F.
3. Pour une liste (type de monture, sens, micropas), Nord et Sud déplacent la surbrillance, F enregistre.
4. Pour un nombre (réduction, jeu, latitude), Ouest augmente, Est diminue, F enregistre. Selon l'écran, le pas d'incrément est fixe.
5. Le message **Sauvegardé** confirme l'écriture. **ERREUR** ou l'échec de la commande signifie que la valeur n'a pas été prise.
6. Est ou f remonte d'un niveau sans modifier la valeur en cours d'édition.

Certains changements exigent un redémarrage : type de monture, choix de la monture 0 ou 1, activation des moteurs ou des codeurs, ergonomie, Wi-Fi. L'écran affiche alors **App. sur boutton** / **pour redémarrer**. Appuyez sur une touche et laissez l'ensemble redémarrer. Ne coupez pas l'alimentation au milieu de ce message.

---

## 6. Première mise en route

À la première mise sous tension, l'unité principale se considère en position maison. Ce n'est pas encore une monture alignée sur le ciel : c'est seulement l'origine mécanique.

### Ce qu'il faut régler avant de pointer

1. **Type de Monture** : Équ. Allemande, Équ. Fourche, Altaz. ou Altaz. Fourche.
2. La **réduction**, le **nombre de pas par tour** et les **micropas** de chaque moteur (chapitre 9).
3. Le **sens de rotation** (chapitre 8).
4. Le **lieu** (latitude, longitude, altitude) et l'**heure** (chapitre 10).

Sans le lieu et l'heure, le suivi sidéral peut tourner, mais les coordonnées équatoriales et les catalogues ne correspondent pas au ciel.

### Position maison par défaut

- Monture équatoriale : tube vers le pôle céleste. Sur une allemande, la barre de contrepoids est en bas.
- Monture altazimutale : hauteur 0° (tube horizontal) et azimut 180° (tube vers le sud). La base doit être de niveau.

Placez le tube dans cette position **avant** d'allumer, ou allumez puis amenez-le à la maison avec les boutons et enregistrez la position (chapitre 11).

On peut se passer de la maison pour un premier essai : amenez le tube à la main sur un objet évident (Lune, planète, étoile brillante), faites une **Synchro**, puis utilisez le goto. L'alignement à deux étoiles, lui, part de la position maison, ou d'une étoile dont on indique le côté de pilier.

### Contrôle rapide proposé par le groupe

1. Goto vers **Home**, puis, freins desserrés s'il le faut, placez le tube vers le pôle.
2. Goto vers une étoile brillante, centrez-la, faites une **Synchro**.

C'est suffisant pour une soirée visuelle si la mise en station mécanique est déjà bonne. Pour un modèle qui absorbe une erreur de mise en station, passez par l'alignement du chapitre 13.

---

## 7. Type de monture, réfraction, pôle

**Param. Télescope → Monture**

| Ligne | Contenu |
|-------|---------|
| Monture | Choisit le jeu 0 ou le jeu 1. Redémarrage |
| Type de Monture | Équ. Allemande, Équ. Fourche, Altaz., Altaz. Fourche. Redémarrage |
| Moteurs | Mécanique, vitesses, suivi |
| Encodeurs | Codeurs de position |
| Limites | Horizon, zénith, axes, méridien |
| Réfraction | Voir ci-dessous |
| Réticule | Luminosité du réticule polaire, si la sortie est câblée |

**Réfraction → Goto** : ON ou OFF. Activée, la réfraction atmosphérique est prise en compte dans le pointage (formule de Saemundsson à l'aller, Bennett au retour).

**Réfraction → Alignement du Pôle** (montures équatoriales seulement) :

- **Pôle Apparent** si vous utilisez un chercheur polaire, qui montre le pôle tel que la réfraction le déplace.
- **Pôle Vrai** si la mise en station vise l'axe polaire vrai, déjà corrigé de la réfraction.

Sur une altazimutale, seule la réfraction du goto est proposée : il n'y a pas de pôle mécanique à choisir.

---

## 8. Sens de rotation

Le test se fait à vitesse lente ou moyenne, tube à peu près en position maison, en regardant dans le chercheur ou à l'oculaire.

**Équatoriale allemande**

- Sud doit envoyer le tube vers l'ouest. S'il part vers l'est, inversez la rotation de l'axe 1, vérifiez à nouveau.
- Est doit envoyer le tube vers l'est. Sinon, inversez encore l'axe concerné et revérifiez.

**Équatoriale à fourche**

- Sud doit envoyer le tube vers le sud. Sinon, inversez l'axe 2.
- Est doit envoyer le tube vers l'est. Sinon, inversez l'axe 1.

**Altazimutale** : Est et Ouest commandent l'azimut (axe 1), Nord et Sud la hauteur (axe 2). Le tube doit partir dans le sens annoncé par le bouton.

Le réglage est dans **Monture → Moteurs → Moteur 1** ou **Moteur 2 → Rotation** : **Directe** ou **Inversée**.

L'axe 1 est l'ascension droite ou l'azimut. L'axe 2 est la déclinaison ou la hauteur.

---

## 9. Moteurs, réduction, courant, jeu

**Monture → Moteurs**, moteurs activés :

Afficher Paramètres, Moteur 1, Moteur 2, Accélération, Vitesse, Suivi, Tps. Attente, Désactive.

**Afficher Paramètres** rappelle, pour chaque axe, le sens, la réduction, les pas par tour, les micropas, le jeu en secondes d'arc, et les courants bas et haut. Parcourez ces écrans et notez-les avant toute modification.

### Réduction

C'est le nombre de tours moteur pour un tour d'axe, dents comprises.

Exemple : roue de 360 dents et vis sans fin, sans autre démultiplication : réduction 360. Une poulie 2:1 en amont de la vis porte la réduction à 720. Un réducteur planétaire 5:1 sur le même axe la porte à 1800.

La valeur acceptée va de 1 à 60000, avec trois décimales. Saisissez le rapport total, pas seulement le nombre de dents de la roue si un autre étage existe.

La commande **Verif. engren.** (chapitre 18) mesure ce rapport sur le ciel et propose une valeur corrigée.

### Pas par tour et micropas

**Nb. Pas par Rot.** est le nombre de pas entiers du moteur, en général 200 ou 400. La saisie va de 20 à 400.

**Micropas** : 2, 4, 8, 16 (~256), 32, 64, 128, 256. Le choix **16 (~256)** est celui que le groupe retient le plus souvent sur les drivers TMC : 16 micropas réels, interpolation interne vers 256. Monter au-delà de 16 sans besoin réduit la vitesse maximale de pointage plus qu'il n'améliore le suivi.

### Courant

**Amp. Bas** sert aux vitesses lentes. **Amp. Haut** sert au pointage rapide. La saisie se fait par pas de 100 mA crête, de 200 mA à 2800 mA.

Montez le courant si le moteur décroche au goto ou grogne en charge. Descendez-le si le moteur chauffe sans charge utile. Le courant haut doit rester dans la limite du moteur et du driver.

### Jeu

**Jeu** est la compensation au changement de sens, en secondes d'arc, de 0 à 999.

**Vitesse Jeu** règle la vitesse de rattrapage de ce jeu, de 16 à 64.

Réglez le jeu après que la réduction et le sens sont justes. Trop de jeu fait sauter l'étoile à chaque inversion. Trop peu laisse un décalage visible quand on revient en arrière avec les boutons.

### Mode silencieux

**Silent** ON utilise le mode silencieux du driver (StealthChop sur TMC). OFF privilégie le couple à haute vitesse. Si le goto décroche seulement en Silent, repassez sur OFF pour les essais, puis remontez le courant haut avant de réessayer Silent.

### Accélération

**Accélération** est la distance, en degrés, sur laquelle la monture atteint la vitesse max de pointage. De 0,1° à 25°. Une valeur trop petite secoue la monture. Une valeur trop grande allonge chaque goto.

### Temps d'attente

**Tps. Attente** est le délai de stabilisation en fin de pointage, avant que le suivi reprenne. Il laisse retomber les vibrations.

### Désactiver les moteurs

**Désactive** coupe la commande des moteurs et demande un redémarrage. Utile pour déplacer le tube à la main sans lutter contre le maintien. **Active** les réarme. Tant que les moteurs sont coupés, le menu se réduit à cette seule ligne.

---

## 10. Heure, lieu, GNSS

**Param. Télescope → Heure & Site**

### Heure

- **Horloge** : heure civile affichée.
- **Fuseau Horaire** : décalage par rapport à UTC, heure d'été comprise. En France métropolitaine : +1 h en hiver, +2 h en été.
- **Date**
- **Heure GNSS** : lecture de l'heure satellite, si un récepteur est présent.

L'unité principale en déduit le temps sidéral local. Une heure fausse d'une minute décale le pointage d'environ 15' en ascension droite.

### Site

Trois lieux peuvent être mémorisés.

- **Latitude** : positive au nord, négative au sud.
- **Longitude** : la convention affichée est celle de l'écran de saisie ; vérifiez le signe avec un objet connu après la première synchro. Une longitude inversée se voit tout de suite : le goto tombe systématiquement à l'opposé en azimut.
- **Altitude du Site** : altitude du lieu, en mètres. Elle entre dans la réfraction.
- **Selection Site** : lieu actif.

### Synchro GNSS

**Synchro GNSS** copie l'heure et le lieu du récepteur vers l'unité principale. Sans antenne, ou à l'intérieur, l'écran indique **Pas de GNSS**. La synchro automatique est proposée lorsque la monture est à la maison ou au parc et que le récepteur a un fix.

---

## 11. Maison et parc

La **position maison** est l'origine des axes. La **position parc** est l'endroit où l'on range le tube en fin de nuit, moteurs encore informés de leur angle, pour retrouver le ciel au rallumage sans refaire la mise en station.

| | Maison | Parc |
|---|--------|------|
| Rôle | Origine mécanique, départ de l'alignement | Rangement |
| Équatoriale | Vers le pôle, contrepoids en bas sur une allemande | Là où le tube est en sécurité |
| Altazimutale par défaut | Hauteur 0°, azimut 180° (sud) | Au choix |

**Param. Télescope → Parc et Maison**

- **Déf. Pos. Parc** : la position actuelle devient le parc.
- **Déf Pos. Maison** : la position actuelle devient la maison. À n'utiliser qu'une fois le tube réellement dans la pose que vous voulez comme origine.
- **Reset Pos. Maison** : revient à la maison d'usine du type de monture.

**Action Télescope → Goto → Home** ou **Parc** envoie le tube à cet endroit. **Synchro → Home** ou **Parc** déclare que le tube y est déjà, sans le déplacer : à réserver au cas où vous l'y avez mis à la main.

Quand la monture est parquée, le menu d'action ne propose que **Déparc**. Déparquez avant tout déplacement.

L'icône maison ou parc à droite de l'écran confirme l'état. Si elle est absente alors que le tube vous semble à la maison, la position mémorisée et la position réelle ont divergé : soit vous ramenez le tube, soit vous redéfinissez la maison.

---

## 12. Vitesses

Deux endroits distincts :

- **Shift + Nord → Réglage Vitesse** choisit la vitesse des boutons pour la séance : Guidage, Lente, Moyenne, Rapide, Max. La ligne en surbrillance est le cran actif. Le détail des barres est au chapitre 4.
- **Moteurs → Vitesse** définit ce que valent ces cinq crans, plus la vitesse retenue au démarrage.

![Réglage Vitesse](images/screens/menu_speed.png)

| Cran | Réglage | Plage |
|------|---------|--------|
| Guidage | fraction de la vitesse sidérale | 0,10× à 1,00× |
| Lente, Moyenne, Rapide | multiple de la vitesse sidérale | 1× à 255× |
| Max | vitesse de goto | 60× à 3600×, par pas de 60 |
| Vitesse standard | cran actif à la mise sous tension | l'un des cinq |

La vitesse de guidage est aussi celle de la prise autoguideur ST-4.

Pour centrer une étoile : Rapide ou Moyenne pour l'amener dans le chercheur, Lente dans l'oculaire, Guidage pour la poser au centre sans la dépasser.

---

## 13. Le suivi

**Action Télescope → Suivi**

Si le suivi est arrêté, la seule ligne est **Démarrer le Suivi**.

S'il tourne :

- **Arrêter le Suivi**
- **Sidéral** : étoiles
- **Lunaire** : Lune
- **Solaire** : Soleil
- **Objet** : taux mémorisé pour une comète ou un objet qui dérive

On ne change pas le taux pendant un pointage. L'écran le signale par **Suivi en cours** / **Non modifiable**.

### Dérive (comètes, objets lents)

**Moteurs → Suivi → Vitesse de derive**

- **Asc. Droite** : en secondes de temps par intervalle sidéral (unité affichée `s/SI`), de −2 à +2.
- **Déclinaison** : en secondes d'arc par intervalle sidéral (`"/SI`), de −2 à +2.

Réglez les deux d'après l'éphéméride, démarrez le suivi, puis choisissez **Objet**. Remettez Sidéral pour revenir aux étoiles. Une dérive nulle sur les deux axes et le taux Objet reviennent au suivi sidéral.

### Réfraction sur le suivi

**Moteurs → Suivi → Réfraction** : ON ou OFF. Distincte de la réfraction du goto. Elle corrige la vitesse de suivi pour la hauteur de l'objet.

### Correction de suivi

**Moteurs → Suivi → Corr. du Suivi**, sur les montures équatoriales :

- **Asc. Droite** : la correction d'alignement et de réfraction n'agit que sur l'axe horaire.
- **Les deux** : les deux axes sont corrigés.

Sur une altazimutale la correction se fait toujours sur les deux axes, et cette ligne n'apparaît pas. L'icône de suivi change quand la correction est armée (un axe, ou les deux).

Le groupe recommande la correction sur les deux axes dès qu'un alignement à deux étoiles a été enregistré, et sur l'ascension droite seule si la mise en station polaire est déjà bonne et que vous ne voulez pas de dérive en déclinaison introduite par le modèle.

---

## 14. Goto, synchro, catalogues

**Shift + Est → Action Télescope**, monture déparquée.

| Ligne | Rôle |
|-------|------|
| Goto | Amène le tube sur la cible |
| Pushto | Indique la direction, sans moteur, si les codeurs sont actifs |
| Synchro | Déclare que l'objet choisi est au centre de l'oculaire |
| Aligner | Modèle à deux étoiles |
| Verif. engren. | Mesure la réduction |
| Suivi | Démarre, arrête, choisit le taux |
| Côté du Pilier | Indique le côté, monture allemande |
| Sauver RADEC | Mémorise la position courante comme cible utilisateur |
| Verrouiller | Bloque les actions jusqu'à Déverrouiller |
| Spirale | Recherche en spirale |

Le menu **Goto** et le menu **Synchro** proposent :

Catalogues, Système Solaire, Coordonnées, Déf. par Util., Home, Parc.

Le goto ajoute **Retournement**.

Le push-to propose Catalogues, Système Solaire, Coordonnées et Déf. par Util., et seulement si les codeurs sont actifs. Sinon l'écran affiche **Encodeurs** / **pas Connecté**.

### Système solaire

Soleil, Mercure, Vénus, Mars, Jupiter, Saturne, Uranus, Neptune, Lune. Les positions sont apparentes, pour le lieu et l'heure courants.

Ne pointez le Soleil qu'avec un filtre plein ouvert adapté. Le contrôleur ne sait pas si un filtre est en place.

### Coordonnées

- **J2000** : ascension droite et déclinaison de l'équinoxe 2000.0, comme dans la plupart des catalogues imprimés.
- **JNow** : coordonnées apparentes de la date.
- **Alt Az** : azimut et hauteur.
- **N, S, E, O** : les quatre points cardinaux à l'horizon (hauteur 0°). Pratique pour vérifier le sens et le niveau d'une altazimutale.

L'unité principale applique la précession, la nutation et l'aberration entre J2000 et la date, puis la transformation équatorial vers horizontal selon la latitude et le temps sidéral.

### Catalogues

La liste dépend de la version de firmware chargée dans la raquette. On y trouve en pratique les étoiles brillantes, Messier, et selon la compilation NGC, IC, Caldwell, Herschel, Collinder, des doubles (STF, STT) et des variables (GCVS).

### Filtres

**Goto → Catalogues → Filtres**. Le titre de l'écran est **FiltresAutoriser**. Les noms de lignes restent en anglais, même sur la raquette française. Un signe **+** de chaque côté d'une ligne veut dire que ce filtre est retenu. **Reset Filtres** les enlève tous.

| Ligne | Ce qu'elle garde |
|-------|------------------|
| Above Horizon | La hauteur minimale. **Filtre Horizon** propose **> Horizon**, puis **> 10 deg.**, **> 20 deg.**, et ainsi de suite jusqu'à **> 70 deg.** |
| Constellation | Une seule constellation. **Filtre par Const.** commence par **Toutes**, puis les abréviations : Her pour Hercule, Lyr pour la Lyre, And pour Andromède, Ori pour Orion |
| Type | Un type d'objet du ciel profond. **Filtre par Type** commence par **Toutes**. Les noms restent ceux de la base : Galaxy, Globular Clstr, Planetary Nebula, Open Cluster, Nebula |
| Magnitude | La magnitude la plus faible encore affichée. **Filtre Magnitude** propose **Toutes**, puis 10, 11, 12, 13, 14, 15 et 16 |

Les objets déjà sous l'horizon n'apparaissent pas. Choisir **> Horizon** change donc peu de chose. **> 30 deg.** ne garde que ce qui est assez haut pour un oculaire confortable, loin des arbres et de la turbulence.

La magnitude se lit à l'envers de l'intuition : un petit nombre est un objet brillant. **10** retire tout ce qui est de magnitude 10 ou plus faible. **Toutes** ne coupe rien. Sous un ciel clair avec un petit instrument, 10 ou 11 suffit. La limite 16 ne sert que si le catalogue et le ciel suivent.

**Type** ne change la liste que dans un catalogue du ciel profond (Messier, NGC, et les autres). Sur les étoiles brillantes, il ne retire rien.

Si la raquette a été compilée avec les doubles ou les variables, deux lignes de plus apparaissent, encore en anglais :

- **Dbl* Min Sep.** et **Dbl* Max Sep.** : séparation angulaire, de 0,2" à 100". Le minimum doit rester inférieur au maximum, sinon l'écran affiche **Min Sep must** / **be < Max Sep.**
- **Var* Max Per.** : période maximale, de 0,5 jour à 100 jours. Au-delà, l'étoile variable est masquée. **Off** retire le filtre.

Quand un filtre retire vraiment des objets, le titre du catalogue s'entoure de points d'exclamation, par exemple **!Goto Messier!**. S'il ne reste rien, l'écran dit **No Object** : élargissez un cran, ou **Reset Filtres**.

Nord et Sud font défiler les objets qui restent. F lance le goto ou la synchro selon le menu d'où vous venez. Un exemple complet, M13 puis M31, est au chapitre 25.

### Centrer puis synchroniser

1. Goto sur l'objet.
2. À l'oculaire, amenez l'objet au centre avec les boutons, vitesse lente puis guidage.
3. **Synchro** sur le même objet.

La synchro recale la position. Elle ne construit pas le modèle d'alignement : un seul objet laisse subsister l'erreur dès qu'on s'en éloigne. Pour un modèle, utilisez **Aligner**.

**Sauver RADEC** garde les coordonnées actuelles. **Goto → Déf. par Util.** y retourne.

### Spirale

**Spirale** demande un champ visuel, de 1' à 3°. Le tube décrit une spirale de ce diamètre pour retrouver un objet tombé juste à côté du champ. Shift long arrête le mouvement, comme pour un goto.

### Verrou

**Verrouiller** réduit le menu d'action à **Déverrouiller**. Les boutons de direction restent utilisables. Pratique quand on passe la raquette à quelqu'un qui ne doit pas lancer un goto.

---

## 15. Alignement

L'alignement calcule la transformation entre le ciel et les axes. La méthode est celle de Taki (deux directions mesurées), complétée par une rotation la plus proche au sens des moindres carrés. Le résultat est une matrice 3×3. Elle sert au pointage, à la synchro, au suivi corrigé et au contrôle de hauteur.

**Action Télescope → Aligner**

Tant qu'aucun modèle n'est enregistré, monture équatoriale :

- **2 Étoiles**
- **2 étoiles méca.**
- **Ordinateur Alignement**

Sur une altazimutale, la ligne mécanique n'est pas proposée.

Une fois le modèle en place, s'ajoutent **Sauver**, **Effacer** et **Show align. error**.

### 2 Étoiles, depuis la maison

1. Le tube est en position maison. L'écran le rappelle : **La monture doit être en position Maison.**
2. Choisissez **Home** quand l'écran demande le mode.
3. L'unité principale accepte le départ. Choisissez la première étoile dans la liste (étoiles nommées, au-dessus de l'horizon).
4. Le tube part. Le message **Pointe vers** puis **Recentre** s'affiche.
5. Centrez l'étoile. Un appui long sur Shift valide l'étoile (**Étoile ajoutée**).
6. Choisissez la seconde étoile, éloignée de la première en angle horaire et en déclinaison. Même recentrage, même appui long.
7. En cas de succès, le modèle est calculé. **Sauver** l'écrit en mémoire. Sans sauvegarde, il est perdu au prochain parc ou à la coupure, selon que vous parquez ou non : le wiki du groupe rappelle de parquer en fin de procédure pour conserver le résultat. La commande **Sauver** du menu Aligner fait cette écriture explicitement.

**Effacer** oublie le modèle. La monture revient à l'hypothèse « mécaniquement juste, à partir de la maison ».

### 2 Étoiles, depuis une étoile

Si la maison n'est pas accessible (tube déjà sur le ciel) :

1. Choisissez **Étoile** au lieu de **Home**.
2. Indiquez le **Côté du Pilier** (Est ou Ouest). Sur une fourche ou une altazimutale, indiquez le côté qui correspond à la pose réelle.
3. **Synchro** sur une étoile centrée. Cette étoile devient la première référence.
4. La procédure enchaîne sur la seconde étoile, comme ci-dessus.

### 2 étoiles méca.

Réservé aux équatoriales allemande et à fourche. Même déroulement, mais le calcul force la cohérence avec le pôle mécanique de la monture. À utiliser quand la mise en station polaire est le référence, et que vous voulez un modèle qui ne « tord » pas le pôle. Le départ est encore **Home** ou **Étoile**.

### Depuis un ordinateur

**Ordinateur Alignement** attend les étoiles envoyées par l'application ou par un logiciel via le protocole. L'écran passe en **Align. distant** et affiche le nom de l'étoile demandée. Le recentrage et l'appui long sur Shift restent faits à la raquette, au centre de l'oculaire.

### Quelle paire d'étoiles

Choisissez deux étoiles brillantes, bien au-dessus de l'horizon, séparées d'au moins une quarantaine de degrés, ni toutes deux près du pôle ni toutes deux près de l'horizon. Une étoile à l'est et une à l'ouest, avec des déclinaisons différentes, donnent un modèle stable. Évitez de valider une étoile mal identifiée : une seule erreur de catalogue se reporte sur tout le ciel.

**Show align. error** affiche l'écart angulaire résiduel du modèle. Un écart de plusieurs dizaines de minutes d'arc invite à refaire la procédure, pas à compenser avec du jeu moteur.

### Ce que l'alignement ne corrige pas

Il ne remplace pas une réduction fausse, un sens inversé, ni une heure fausse. Si le premier goto tombe à des degrés de la cible, reprenez les chapitres 8 à 10 avant de relancer un alignement.

---

## 16. Monture allemande

Le tube peut être d'un côté ou de l'autre du pilier. L'écran indique le côté. Un goto qui traverserait le méridien au-delà des limites déclenche un **Retournement** : le tube passe de l'autre côté, l'ascension droite et la déclinaison de l'objet restant les mêmes.

**Goto → Retournement** le demande tout de suite. Si la pose d'arrivée est hors limites, l'écran affiche **Retournement** / **Impossible**.

**Côté du Pilier** force le côté Est ou Ouest quand la monture a perdu cette information (desserrage des freins, déplacement à la main). Ensuite le menu demande une synchro sur une cible : sans elle, le côté affiché et le ciel ne concordent pas.

### Limites de méridien

**Limites → Équ. Allemande**

- **Méridien Est** et **Méridien Ouest** : de −45° à +45°. Ils autorisent le suivi un peu au-delà du méridien avant le retournement, pour ne pas couper une pose au passage.
- **Sous le Pôle** : angle horaire maximal, de 9 h à 12 h. Il empêche le tube ou la barre de contrepoids de venir dans le pied quand on suit un objet sous le pôle.

Réglez ces valeurs pour votre mécanique (longueur du tube, hauteur des pieds), pas pour le catalogue. Un objet refusé s'annonce par **Hors Limites**, **Sous l'horizon**, **Proche du zénith** ou **Limite Méridien**.

---

## 17. Monture altazimutale

La base doit être horizontale. Un niveau à bulle sur le socle fait partie de la mise en station : l'alignement à deux étoiles absorbe un résidu, pas une base penchée de plusieurs degrés.

Maison par défaut : tube horizontal vers le sud. Vous pouvez en définir une autre avec **Déf Pos. Maison** une fois le tube dans la pose voulue.

Le suivi corrige les deux axes en permanence, parce qu'aucun axe n'est parallèle à l'axe du monde. L'heure et le lieu sont indispensables : sans eux, la rotation du champ et le suivi sont faux même si le tube a été synchronisé sur un objet.

La ligne **Sous le Pôle** ne concerne pas ce type de monture. Les limites utiles sont l'horizon, le zénith et les butées d'axes.

Près du zénith, un petit déplacement au ciel demande un grand déplacement en azimut. La **Limite Zénith** (chapitre suivant) évite cette zone.

---

## 18. Limites

**Monture → Limites**

| Ligne | Sens | Plage de saisie |
|-------|------|-----------------|
| Horizon | Hauteur minimale d'un goto | −10° à +20° |
| Zénith | Hauteur maximale | 60° à 91° |
| Axes | Butées mécaniques des deux axes | en degrés, après affichage de la position courante |
| Équ. Allemande | Méridien et sous le pôle | voir chapitre 16 |

**Horizon** à 0° refuse tout ce qui est sous l'horizon. Une valeur négative autorise un léger passage sous l'horizon, par exemple depuis un site dégagé en contrebas. Une valeur positive tient le tube au-dessus d'un mur ou d'une haie.

**Axes** affiche d'abord la position, puis **Axe 1 Min.**, **Axe 1 Max.**, **Axe 2 Min.**, **Axe 2 Max.** Placez le tube à la main, ou au moteur lentement, contre chaque butée réelle, lisez l'angle, et reculez la limite d'un ou deux degrés pour ne pas taper la mécanique.

Un goto refusé n'est pas une panne. Lisez le message : **Sous l'horizon**, **Proche du zénith**, **Hors Limites**, **Limite DEC**, **Limite AZM**.

---

## 19. Vérification des engrenages

**Action Télescope → Verif. engren.** compare la réduction enregistrée au déplacement réel.

Le firmware demande une synchro sur une étoile A, puis un goto vers B (grand déplacement surtout sur l'axe 1) et vers C (grand déplacement surtout sur l'axe 2). Pour une équatoriale il suggère un grand angle horaire puis une grande déclinaison. Pour une altazimutale, un grand azimut puis une grande hauteur.

À chaque arrivée :

1. Attendez la fin du pointage (**Attente slew...**). Shift long abandonne.
2. Recentrez l'étoile.
3. Appui court sur Shift pour valider.

L'écran affiche l'erreur de chaque étape, en minutes d'arc, et la réduction mesurée. La formule utilisée est : réduction mesurée = réduction enregistrée × déplacement commandé / déplacement vrai. Un déplacement commandé de moins de 5° est rejeté : l'échelle serait trop mauvaise.

Si les deux axes donnent des rapports incohérents, l'écran signale **Axes similaires** : les étoiles choisies n'ont pas assez séparé les axes. Recommencez avec des cibles plus écartées.

Reportez la réduction mesurée dans **Moteur → Réduction** seulement si le sens de rotation est déjà bon et si vous avez recentré avec soin. Une étoile mal centrée se traduit directement en erreur de réduction.

---

## 20. Codeurs et push-to

Les codeurs mesurent la position réelle des axes. Ils servent au push-to (vous poussez le tube, l'écran indique comment rejoindre la cible) et peuvent recaler les moteurs.

**Monture → Encodeurs → Active**, puis redémarrage.

Ensuite :

| Ligne | Rôle |
|-------|------|
| Auto Sync | Recalage automatique moteur vers codeur |
| Calibration | Étalonnage sur une étoile |
| Pulse par deg E1 / E2 | Résolution du codeur |
| Rotation E1 / E2 | Sens du codeur |
| Désactive | Coupe les codeurs, avec redémarrage |

**Pulse par deg** va de 0,50 à 3600 impulsions par degré.

**Auto Sync** : Off, 60', 30', 15', 8', 4', 2', ou On. Le moteur est recalé sur le codeur quand l'écart dépasse le seuil, et seulement hors pointage. **On** recale en permanence dans la tolérance la plus fine prévue par le firmware. **Off** laisse les codeurs informatifs.

### Calibration

1. **Étoile** : démarre l'étalonnage.
2. Pointez et centrez l'étoile de référence comme demandé à l'écran.
3. **terminé** enregistre. **Annule** abandonne.

Le sens **Directe** / **Inversée** de chaque codeur se règle comme celui des moteurs : si le push-to s'éloigne quand vous poussez dans le sens indiqué, inversez ce codeur.

### Push-to

Avec les codeurs actifs, **Action Télescope** contient **Pushto**. Choisissez la cible comme pour un goto. L'écran passe sur la page de distance. Déplacez le tube à la main jusqu'à annuler l'écart. Un appui court sur Shift propose alors de synchroniser les moteurs sur les codeurs.

Sans codeur, cette entrée n'existe pas : le menu d'action est celui du chapitre 14, sans ligne Pushto.

---

## 21. Prise autoguideur

La prise ST-4 de l'unité principale reçoit les quatre directions d'un autoguideur ou d'une caméra de guidage. La vitesse appliquée est la **Vitesse guidage** du chapitre 12.

Le suivi doit être en marche. Une correction ST-4 se superpose au suivi ; elle ne le remplace pas. L'état de guidage (impulsion, ST-4, recentrage) est visible des logiciels connectés, et le déplacement se voit à l'oculaire.

Réglez la vitesse de guidage pour que l'étoile se déplace franchement sur une impulsion d'une seconde, sans traverser tout le capteur. 0,5× sidéral est un point de départ usuel. Descendez vers 0,3× en longue focale, montez vers 0,8× si les corrections n'ont aucun effet.

---

## 22. Mise au point

Si aucun focuser n'est connecté, Shift + F ou Shift + f affiche **Focuser** / **pas Connecté**.

**Action M.A.P.** (Shift + f) : positions nommées déjà enregistrées, **Goto**, **Synchro**, **Parc**, **Verrouiller**. Choisir une position nommée y envoie le focuser. **Synchro** déclare que la position actuelle est la référence. **Parc** range le focuser à sa position de parc.

**Param. Focuser** (Shift + F) :

- **Config** : affichage, position de parc, position max, vitesses manuelle et goto, accélérations.
- **Moteur** : résolution, sens, pas par tour, micropas (4, 8, 16, 32, 64, 128), courant.
- **Info Focuser** : version, redémarrage, réinitialisation usine du focuser seul.

Le verrouillage du focuser évite qu'un appui accidentel ne perde la mise au point pendant une pose.

---

## 23. Wi-Fi, téléphone, ordinateur

**Param. Télescope → Wifi**

- **Allumer la Wifi** ou **Éteindre la Wifi**
- **Affich. mot de passe**
- **Sélection Mode** : jusqu'à trois réseaux station, plus le point d'accès de la raquette
- **Montre IP**
- **Réinit. Usine** de la configuration Wi-Fi de la raquette

Allumer ou éteindre le Wi-Fi demande un redémarrage.

En point d'accès, le téléphone ou l'ordinateur rejoint le réseau de la raquette, avec le mot de passe affiché. En station, la raquette rejoint votre box : l'adresse est celle que **Montre IP** indique, pas une adresse fixe.

Le pont TCP écoute le port **9999** et transporte le même dialogue que le câble USB de l'unité principale. L'application TeenAstro, le pilote ASCOM et les logiciels de planétarium s'y connectent.

L'application reprend le tableau de bord, le planétarium, le goto et l'alignement. Le pilote ASCOM ouvre TeenAstro à Stellarium, Cartes du Ciel, NINA, et aux autres clients ASCOM sous Windows.

Une page web de l'interface permet de saisir les réseaux et le mot de passe sans passer par la raquette. L'adresse est celle affichée par **Montre IP**.

### SkySafari

Il faut SkySafari Plus ou Pro, la version qui commande un télescope. Le téléphone et la raquette doivent être sur le même réseau.

En point d'accès, tel que livré : le réseau s'appelle **TeenAstro**, l'adresse est **192.168.0.1**, le port est **9999**. Le mot de passe d'usine est `password` ; **Affich. mot de passe** montre celui qui est vraiment en mémoire, s'il a été changé. En station, le téléphone est sur votre box et l'adresse est celle de **Montre IP**, toujours sur le port 9999.

Dans SkySafari : Réglages, Télescope, Configuration.

| Réglage | Valeur |
|---------|--------|
| Type de monture | GoTo équatoriale, ou GoTo altazimutale, selon la monture |
| Type de télescope | Meade LX-200 Classic |
| Connexion | Wi-Fi. Pas le mode SkyFi |
| Adresse | 192.168.0.1 en point d'accès, sinon celle de **Montre IP** |
| Port | 9999 |

**Set Time & Location**, s'il est proposé, envoie l'heure du téléphone, et le lieu seulement si la monture est à la maison ou au parc. Si l'heure et le site sont déjà bons sur la raquette, laissez cette case décochée : le téléphone écraserait le lieu. Si vous voulez au contraire prendre le GPS du téléphone, parquez d'abord, ou soyez à la maison, puis cochez la case et connectez.

Connectez. La carte doit montrer le réticule du télescope. Touchez un objet, puis Goto. La raquette affiche l'icône de pointage. Un seul goto à la fois : celui de SkySafari et celui de la raquette s'interrompent l'un l'autre.

Le recentrage se fait encore aux boutons, vitesse lente puis guidage. La synchro peut partir de SkySafari (Sync) ou de la raquette, sur le même objet. L'alignement à deux étoiles du chapitre 15 reste plus simple à la raquette ; SkySafari sert ensuite à choisir les objets.

Pendant qu'un ordinateur commande la monture, la raquette reste utilisable. Évitez deux gotos lancés en même temps : le second interrompt le premier.

### Mise à jour du logiciel

Le wiki du groupe décrit TeenAstroUploader pour Windows.

1. Sauvegardez les paramètres avec TeenAstroConfig avant une mise à jour. Un changement de version peut réinitialiser la mémoire si la clé interne a changé.
2. Câble USB sur le port de l'unité principale seule, monture allumée. La première fois, Windows installe le périphérique Teensy.
3. Dans l'outil, choisissez la carte. Le quatrième écran après la mise sous tension indique le modèle. En cas de doute, lisez le marquage sur la carte.
4. Lancez l'envoi et attendez la fin du chargeur.
5. Pour la raquette par le Wi-Fi : relevez l'adresse avec **Montre IP**, saisissez-la dans l'outil, envoi par Wi-Fi.

Raquette et unité principale doivent provenir de la même publication. Sinon l'écran de version en erreur revient dès le démarrage.

**Info Unité Princip. → Affichage Version** montre le nom et la date du firmware. **Redémarrage** relance l'unité. **Réinit. Usine** efface la mémoire après confirmation NON / OUI.

---

## 24. Mode visiteur

**Param. Raquette → Privilèges** : **Administrateur** ou **Visiteur**.

En visiteur, **Param. Télescope** ne contient plus que **Privilèges**. Les réglages de mécanique, d'heure et de Wi-Fi sont cachés. Les actions de nuit (goto, synchro, suivi, vitesses) restent disponibles. C'est le mode à laisser sur une monture partagée ou en démonstration.

Le choix est mémorisé dans la raquette. Pour revenir en administrateur : Shift + Ouest, **Privilèges**, **Administrateur**.

---

## 25. Une soirée d'observation

L'exemple est une fin d'été, vers 45° de latitude nord : Véga et Altaïr sont hautes, M13 passe encore, M31 se lève. Une autre nuit, gardez les mêmes gestes et changez les noms. Les filtres sont détaillés au chapitre 14, SkySafari au chapitre 23.

### Reconnaître une batterie vide

La raquette n'a pas de jauge. Rien sur l'écran ne dit « batterie faible ». On le voit au comportement, et au contrôle avant la nuit.

Avant d'allumer, un voltmètre sur la batterie est le seul chiffre fiable. Le seuil exact dépend de la chimie (plomb ou lithium) et de la carte : le wiki du groupe donne la tension de votre version. Une batterie qui tient au repos et s'écroule dès qu'un moteur accélère est déjà trop juste.

Pendant la séance, une batterie qui lâche se reconnaît à ceci, alors que les câbles n'ont pas bougé :

- l'écran faiblit, clignote, ou la raquette repart toute seule sur le logo ;
- **pas Connecté**, puis redémarrage, sans qu'on ait touché au câble ;
- un goto en vitesse Max cale, le moteur grogne, le tube s'arrête avant l'objet, et le même déplacement en vitesse lente passe ;
- le Wi-Fi disparaît et **Montre IP** ne répond plus jusqu'au prochain démarrage ;
- le suivi s'arrête et les coordonnées ne correspondent plus au ciel, parce que l'unité principale a redémarré et a perdu le fil.

Ce n'est pas une batterie vide : un écran noir qui se rallume avec Shift est la veille ; **ERREUR** / **version** est un logiciel qui ne correspond pas.

Si ces signes arrivent en cours de goto, arrêtez le mouvement (Shift long). Parquez seulement si les moteurs répondent encore. Rechargez ou changez la batterie avant de continuer : des pas perdus faussent le pointage jusqu'à une nouvelle synchro.

### Mise en route

1. Tube à la position parc, ou à la maison si vous n'avez pas encore de parc. Branchez, allumez, laissez défiler le logo, les versions et l'heure.
2. Vérifiez l'heure et le site (**Heure & Site**). Sans GNSS, c'est vous qui les avez saisis.
3. Si l'icône de suivi n'est pas là : **Action Télescope → Suivi → Démarrer le Suivi**.
4. Si le menu d'action ne propose que **Déparc**, la monture est parquée. Déparquez. L'icône parc doit disparaître.

### Aligner sur deux étoiles

S'il existe déjà un modèle et que la mise en station n'a pas bougé : **Goto → Catalogues → Etoiles brillantes**, choisissez Véga, centrez à la vitesse lente, et ne synchronisez que si l'étoile est entrée dans le champ. Passez ensuite aux objets.

Sinon, alignement à deux étoiles depuis la maison (chapitre 15), avec deux étoiles nommées, hautes, et éloignées l'une de l'autre :

1. Première étoile : Véga, dans la Lyre. Le tube part, l'écran dit **Pointe vers** puis **Recentre**. Centrez. Appui long sur Shift : **Étoile ajoutée**.
2. Seconde étoile : Altaïr, dans l'Aigle. Elle est loin de Véga en angle horaire. Même recentrage, même appui long.
3. **Sauver**. L'icône d'alignement confirme que le modèle est en mémoire.

Une paire trop proche, Véga et Deneb par exemple, donne un modèle fragile. Le chapitre 15 dit comment les choisir.

### Premier objet : M13

M13 est l'amas globulaire d'Hercule, facile dans un chercheur une fois le modèle en place.

1. **Goto → Catalogues → Filtres**.
2. **Above Horizon**, puis **> 30 deg.** M13 doit être assez haut ; sinon le filtre le retirera, et c'est tant mieux.
3. **Constellation**, puis **Her**.
4. **Type**, puis **Globular Clstr**.
5. Revenez aux catalogues, ouvrez Messier. Le titre devient **!Goto Messier!** : les filtres travaillent. Faites défiler jusqu'à M13. F lance le goto.
6. À l'arrivée, vitesse **Lente** dans le chercheur, **Guidage** dans l'oculaire. Ne synchronisez que si vous êtes certain que l'amas est au centre. Une synchro sur le mauvais objet décale tout le ciel.

S'il n'y a **No Object**, M13 est sous 30° ou le type ne correspond pas. **Reset Filtres**, puis recommencez avec **> 10 deg.**

### Deuxième objet : M31

M31, la galaxie d'Andromède, n'est pas dans Hercule. Sans changer le filtre de constellation, elle n'apparaît pas.

1. **Filtres → Constellation → And**.
2. **Type → Galaxy**. Laissez la hauteur à 30° si Andromède est déjà levée, sinon descendez à **> 10 deg.**
3. Messier, M31, F.
4. Le champ est large : vitesse lente, et le chercheur plutôt que le fort grossissement pour la trouver.

Pour une nébuleuse planétaire dans le même coin du ciel que Véga : **Constellation → Lyr**, **Type → Planetary Nebula**, puis M57.

La Lune, Jupiter ou Saturne passent par **Goto → Système Solaire**, sans filtre. Le Soleil uniquement avec un filtre plein ouvert sur le tube. Le contrôleur ne le vérifie pas.

### Fin de nuit

**Goto → Parc**. Attendez l'icône parc, pas seulement l'arrêt des moteurs. Coupez ensuite l'alimentation. Un parc interrompu laisse la position fausse au rallumage.

Si vous avez desserré les freins dans la nuit, refaites au moins une synchro, et un alignement si le tube a beaucoup bougé sans que les codeurs ne suivent.

---

## 26. En cas de problème

| Ce que vous voyez | Piste |
|-------------------|--------|
| **ERREUR** / **version** | Raquette et unité principale de publications différentes |
| **pas Connecté**, puis redémarrage | Câble raquette, ou batterie qui s'écroule, ou unité principale arrêtée |
| L'écran faiblit, la raquette repart sur le logo, un goto cale en vitesse Max | Batterie vide ou trop faible en charge. La raquette n'affiche pas la tension. Chapitre 25 |
| Écran noir, Shift le rallume | Veille, pas une batterie vide |
| Les boutons partent à l'envers | Chapitre 8, sens de rotation |
| Le goto est systématiquement à côté, d'un facteur deux ou plus | Réduction ou nombre de pas. Faites une vérification d'engrenages |
| Le goto est bon près de l'étoile de synchro et se dégrade ailleurs | Alignement absent ou étoile mal identifiée. Heure ou longitude fausse |
| **Sous l'horizon** ou **Hors Limites** sur un objet pourtant visible | Limite d'horizon, mauvais lieu, ou côté de pilier incorrect |
| **Proche du zénith** | Limite zénith, ou objet réellement trop haut pour la mécanique |
| Le moteur décroche au goto | Courant haut trop faible, Silent, accélération trop brutale, vitesse max trop haute |
| Le moteur chauffe à l'arrêt | Courant bas trop élevé |
| L'étoile saute à chaque inversion de bouton | Jeu trop grand, ou jeu nul alors que la mécanique en a |
| L'alignement échoue tout de suite | La monture n'était pas à la maison, ou la première synchro a été refusée |
| **Retournement** / **Impossible** | La pose d'arrivée franchirait une limite de méridien ou d'axe |
| Le push-to s'éloigne de la cible | Sens du codeur inversé, ou impulsions par degré fausses |
| Le Wi-Fi ne montre pas d'adresse | Wi-Fi éteint, mauvais mode, ou redémarrage pas encore fait |

Avant d'effacer la mémoire : notez les paramètres, ou relisez la sauvegarde TeenAstroConfig. La réinitialisation usine est le dernier recours, pas le premier.

---

## Annexe A. Arborescence des menus

Les lignes entre parenthèses n'apparaissent que dans le cas indiqué.

**Shift + Est — Action Télescope**

- (si parqué) Déparc
- Goto — Catalogues, Système Solaire, Coordonnées, Déf. par Util., Home, Parc, Retournement
- (codeurs) Pushto — Catalogues, Système Solaire, Coordonnées, Déf. par Util.
- Synchro — comme le goto, sans Retournement
- Aligner — 2 Étoiles, (équatoriale) 2 étoiles méca., Ordinateur Alignement, puis Sauver, Effacer, Show align. error
- Verif. engren.
- Suivi
- Côté du Pilier
- Sauver RADEC
- Verrouiller
- Spirale

**Shift + Nord — Réglage Vitesse** : Guidage, Lente, Moyenne, Rapide, Max

**Shift + Ouest — Param. Télescope** (administrateur)

- Param. Raquette — Privilèges, Écran, Vitesse Boutons, Ergonomie, Reset
- Heure & Site — Heure (Horloge, Fuseau, Date, Heure GNSS), Site, Synchro GNSS
- Parc et Maison — Déf. Pos. Parc, Déf Pos. Maison, Reset Pos. Maison
- Monture — Monture, Type de Monture, Moteurs, Encodeurs, Limites, Réfraction, Réticule
- Info Unité Princip. — Affichage Version, Redémarrage, Réinit. Usine
- Wifi

**Moteurs → Moteur 1 ou 2** : Afficher Paramètres, Rotation, Réduction, Nb. Pas par Rot., Micropas, Jeu, Vitesse Jeu, Amp. Bas, Amp. Haut, Silent

---

## Annexe B. Lettres grecques

Les catalogues d'étoiles utilisent les lettres de Bayer. Le firmware français nomme le catalogue « Etoiles brillantes ».

| Lettre | Nom | Lettre | Nom |
|--------|-----|--------|-----|
| α | alpha | ν | nu |
| β | bêta | ξ | xi |
| γ | gamma | ο | omicron |
| δ | delta | π | pi |
| ε | epsilon | ρ | rhô |
| ζ | zêta | σ | sigma |
| η | êta | τ | tau |
| θ | thêta | υ | upsilon |
| ι | iota | φ | phi |
| κ | kappa | χ | khi |
| λ | lambda | ψ | psi |
| μ | mu | ω | oméga |

Quelques noms propres fréquents dans ce catalogue : Sirius, Canopus, Arcturus, Véga, Capella, Rigel, Procyon, Bételgeuse, Altaïr, Aldébaran, Spica, Antarès, Pollux, Deneb, Régulus.

Pour l'alignement, une étoile nommée et haute vaut mieux qu'une étoile faible dont on n'est pas sûr.

---

## Annexe C. Calculer la réduction

Réduction = (dents de la roue) × (rapport de tout réducteur ou poulie en amont).

| Montage | Calcul | Valeur à saisir |
|---------|--------|-----------------|
| Vis sans fin, roue 360, moteur en direct | 360 × 1 | 360 |
| Roue 180, poulie 16/8 | 180 × 2 | 360 |
| Roue 144, réducteur 10:1 | 144 × 10 | 1440 |
| Couronne 360, deux étages 3:1 et 2:1 | 360 × 3 × 2 | 2160 |

Le nombre de pas par tour est celui du moteur nu (200 pour 1,8°, 400 pour 0,9°), pas le produit par les micropas. Les micropas se règlent à part.

Résolution approximative au ciel, en secondes d'arc par micropas :

360 × 3600 / (réduction × pas par tour × micropas)

Exemple : réduction 360, moteur 200 pas, 16 micropas → 11,25" par micropas. C'est l'ordre de grandeur du plus petit déplacement, avant interpolation du driver. Ce n'est pas la précision de pointage, qui dépend du jeu, de la flexion et de l'alignement.

---

## Annexe D. Où aller plus loin

- Groupe et wiki : [https://groups.io/g/TeenAstro/wiki/home](https://groups.io/g/TeenAstro/wiki/home). On y trouve la première mise en route, les menus illustrés, les cartes, l'outil de sauvegarde et la procédure de flashage.
- Documentation technique du dépôt : [docs/README.md](../README.md) (architecture, suivi, protocole). Elle s'adresse à qui modifie le logiciel, pas à la conduite de la nuit.
- Manuel FS2, pour l'esprit d'origine : [anleit_f.pdf](https://www.astro-electronic.de/anleit_f.pdf), Astro-Electronic, Michael Koch.

Les cartes, les tensions d'alimentation et les faisceaux moteurs ne sont pas les mêmes d'une version à l'autre. Pour le câblage, partez de la page de votre carte dans le wiki, puis revenez ici pour l'usage.
