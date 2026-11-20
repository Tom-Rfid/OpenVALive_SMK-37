# 02 — Le SMK-37 (Elite et Pro)

OpenVALive tourne sur les claviers **M-VAVE SMK-37 Elite et Pro** : la même appli que le FM-1 (4 pistes, boucles,
CAPTURE, scènes, Morph, macros, effets de jeu, Creative Engine), avec ce que le SMK-37 a en plus : 37 touches, 16
pads RVB sensibles à la pression, 4 faders, les molettes, une pédale, un encodeur MASTER et une **prise MIDI OUT**.

**État (17 novembre 2026)** : le firmware est compilé par la CI pour l'Elite et le Pro. L'appli est testée sur
l'ordinateur avec un faux SDK et un faux panneau (47 vérifications : touches, pads, faders, molettes, pédale,
boutons, MIDI, sauvegarde). **Rien n'a encore tourné sur un vrai SMK-37** : le premier essai est sur l'Elite du
propriétaire, avec le plan de test plus bas.

## Avant d'installer

- **Licence** : le firmware SMK-37 est en **GPL-3.0** (avec l'exception de liaison du SDK), parce qu'il est bâti sur
  `smk37_open`, le kit officiel de M-VAVE. Le cœur et l'appli FM-1 restent sous MIT
  ([17 — publication](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_FR/17-publication.md)).
- **Calibration** : la vélocité de chaque touche et de chaque pad vient de la calibration d'usine. Le scanner de
  M-VAVE la **lit** ; OpenVALive ne l'écrit jamais.
- **Bluetooth** : aucun. Le firmware est compilé sans la pile Bluetooth de M-VAVE (`targets/smk37/no_bluetooth.py` : ~180 Ko de flash et ~10 Ko de RAM rendus, le logo d'accueil avec) ; `build.py --bluetooth` la remet. Le bouton Bluetooth sert au TAP tempo et ouvre MAP CONTROL.
- **Touches à l'écran** : les touches n'ont pas de voyants, alors tant qu'un bouton de couche (SELECT, SEQ, ARP, GLO, SAVE, HOME, FX) est tenu, l'écran montre ses touches à la place de l'oscilloscope, allumées comme les voyants du FM-1 (KEYS ON SCREEN dans les réglages). Couleurs : **jaune** appuyée ; **violet** quelque chose d'actif (une piste qui joue, un effet en cours…) ; **menthe** disponible, pas actif ; **rose** validé au relâché (lâcher le bouton en premier pour le prendre) ; **menthe / rose en alternance** validé au relâché et actif (A VOL en cours…) ; **bleu** les écrans de réglage (SOUNDS, LOOP EDIT, OPTIONS) ; **rouge** une piste coupée (SEL).
- **Touches de batterie** : carte ORIGINAL ou GM ([doc 20 du FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_FR/20-sons-et-touches-batterie.md)) ; les pads en mode DRUMS jouent les notes GM 36 à 49 sur la première piste de batterie, comme les boîtes à rythmes.
- **Tes données M-VAVE Open** : OpenVALive garde son set dans les emplacements de projets **6 à 16** d'Open. Les
  projets 1 à 5, les réglages et les samples d'Open ne sont pas touchés. Sauvegarde-les quand même avec l'atelier
  web avant d'installer.
- **Un paquet par modèle** : `openvalive-smk37-elite-X.Y.Z.fwsc` pour l'Elite, `-pro-` pour le Pro. Le clavier refuse
  le paquet de l'autre modèle.
- **Le chemin du retour** : l'atelier SMK37 OPEN de M-VAVE (<https://open.m-vave.com/smk37-workbench>, Chrome ou
  Edge) : *Restore factory V16* (firmware d'origine) ou *Switch to Open firmware*.

## Installer (Windows)

1. Dans la dernière [release](https://github.com/Tom-Rfid/OpenVALive_SMK-37/releases) (ou la pré-release `nightly`),
   télécharge **`OpenVALive-SMK37-Elite-X.Y.Z.zip`** (ou `-Pro-`) et dézippe-le.
2. Branche le SMK-37 en USB (câble **data**), allume-le, ferme l'atelier web, ton DAW et tout programme MIDI.
3. Double-clique sur **`FLASH-SMK37.bat`** (ou clic droit sur `flash-smk37.ps1` → *Exécuter avec PowerShell*).
   Le script prépare Python 3.12 la première fois, affiche l'identité du clavier, puis installe.

En ligne de commande (Windows, Linux, macOS) :

```bash
py -3.12 fm1_install.py --port SMK --info
py -3.12 fm1_install.py --port SMK --force --no-resources openvalive-smk37-elite-0.0.3.fwsc
```

`--force` : le paquet porte le chargeur de mise à jour de M-VAVE (celui du kit), pas celui de Felucca ; le clavier
vérifie le paquet avant d'écrire. `--no-resources` : rien d'autre n'est écrit.

Si l'installation s'arrête au milieu, le clavier reste en mode mise à jour : relance la même commande, elle reprend.
Si plus rien ne répond, l'atelier web de M-VAVE sait réinstaller un firmware depuis le mode mise à jour.

## Les commandes

| SMK-37 | OpenVALive |
|---|---|
| **37 touches** (Do3 à Do6) | jouent la piste sélectionnée, avec la vélocité ; OCT− / OCT+ décalent la ligne sélectionnée d'une octave (T1..T4 et A chacune la leur ; un son USER ramène son octave) |
| touches blanches + un bouton de couche tenu | les 16 « touches de couche » du FM-1 (GLO scènes, ARP Creative, SELECT pistes, SEQ effets, SAVE sons) : 1re touche blanche = 1. Sur une piste, ARP / SEQ / GLO + touche blanche affiche le geste : relâcher ARP / SEQ / GLO d'abord l'applique (menthe), la touche d'abord l'annule |
| **16 pads** : 2 pads de réglage à droite, 14 pads de jeu | en haut à droite **MODE** : une tape fait le tour **CLIPS** (menthe) → **DRUMS** (rouge) → **PERFORM** (orange) → **SCÈNES** (bleu) → **MIDI** (blanc) ; le pad prend la couleur du mode. En bas à droite **REC** : REC + un pad, **REC relâché en premier** = garder dans ce pad (un clip, une scène), le pad relâché en premier = abandon (titre rouge). **PAD/STEP** fait aussi le tour des modes (voyant éteint : PERFORM) |
| pads en **CLIPS** | un clip = une phrase jouée sur la ligne choisie (une piste ou A), avec son son. **REC + pad** l'arme (le pad clignote rouge 4 fois par seconde, la ligne passe vert sombre à l'écran) ; la **première note** lance la prise (vert vif qui remplit la ligne, toute la largeur = 4 mesures, le maximum) ; **n'importe quel pad, MODE, REC ou un bouton** la termine, son propre pad la termine et la joue. Longueur arrondie au temps, au plus **4 mesures / 48 notes**, 14 clips. **Une tape = SINGLE** : une fois, tout de suite, depuis sa première note (encore : depuis le début). **MODE + pad**, ou le pad tenu puis MODE = **LOOP** : verrouillé, en boucle, depuis le temps suivant (clignote vert / violet, une demi-seconde chacun) ; une tape le libère. **REC + un pad plein** : REPLACE (l'ancien reste jusqu'à la première note). **SHIFT + pad** : effacé. Couleurs : éteint = vide, rouge = batterie, vert = acid, rose = son USER, bleu = les autres ; vif = en lecture. Les clips jouent le son actuel de la ligne, ne s'enregistrent pas dans la boucle et ne sont pas sauvés (hors tension : perdus) |
| pads en **DRUMS** | le **kit de batterie propre aux pads** (pas une piste) : 14 sons, Do2 (grosse caisse) en bas à gauche, comme les boîtes à rythmes ; sortie sur le **canal MIDI 10**. **REC tenu** : les cinq kits sur les cinq premiers pads (tous, même les cachés ; vif = celui des pads) ; REC + pad, REC lâché en premier = ce kit, gardé (SAVE). La pression = un **roulement** calé sur la grille, plus rapide en appuyant plus fort : 1/8, 1/16, 1/32 ; en relâchant, il s'arrête. Pas enregistré dans les boucles |
| pads en **PERFORM** (par défaut) | 14 effets de jeu : boucles 1 mesure à 1/16, SLICER, filtre fermé / ouvert, delay, crush, break, solo, tout coupé, tout arrêter (DRIVE et DRUMS ONLY restent sur les touches ; LOOP 1/32 par OCT+ depuis 1/16). Tenir = tant que tenu ; tape courte = verrouillé. **Pression** sur un filtre / delay / crush tenu : effleurer = 40 %, appuyer fort = 100 % |
| pads en **SCÈNES** | rangée du haut + le pad en bas à gauche : scènes 1–8 (à la mesure suivante en lecture ; vif : la dernière, pâle : enregistrée, sombre : vide) ; **REC + pad** (ou SHIFT + pad) la SAUVE (rose) ; puis mute / unmute des pistes 1–4, les quatre (rouge qui clignote vite quand tout est coupé), **A VOL** (vert qui respire pendant qu'il tourne) |
| pads en **MIDI** | 14 notes depuis Do2 sur le **canal MIDI 16** seulement (USB et prise MIDI OUT), aucun son interne : pour une boîte à rythmes ou un sampler |
| pads + un bouton de couche tenu | les 16 emplacements de la couche, quel que soit le mode : GLO + pads 1–8 rappellent les scènes, 9–16 les mémorisent… |
| HOME + 4e touche noire (Sol#3) | **LOOP EDIT** : les notes de la boucle une par une (SELECT, une touche = hauteur, K1–K4 position / longueur / vélocité / hauteur, REC efface, EDIT ajoute, EDIT tenu + touches : un accord) |
| **les 12 boutons de droite** | dans l'ordre du FM-1. En haut : ARP, NOTE REPEAT = SEL, SCALE = ENV (annuler), CHORD = LFO (refaire), GLOBE = EDIT, BT = GLO. En bas : PATCH = HOME, PARA = SAVE, FX, SEQ, PLAY, REC |
| **boutons de gauche** | KNOB BANK = SHIFT, FADER BANK ; **<** = TAP (tenu : MAP CONTROL), **>** = le mode des pads |
| **8 boutons rotatifs** | K1 la piste (SELECT), K2 le son (PRESET), K3–K6 les K1–K4 du FM-1, K7 le tempo, K8 MASTER |
| **pads** | leur propre mode seulement (CLIPS / DRUMS / PERFORM / SCÈNES / MIDI) : ils ne suivent jamais un bouton de couche tenu (les touches et l'écran le font) |
| **4 faders** | volume des pistes 1 à 4 (100 % aux 4/5 de la course) |
| **FADER BANK** | fait tourner trois banques : **volumes** → **macros** ENERGY, SPACE, TENSION, DENSITY (voyant allumé) → **USER**, ce que MAP CONTROL leur donne (voyant qui clignote) |
| **HOME** + touche blanche 8 (Do4), HOME relâché d'abord | **SOUNDS** : les sons que PRESET propose. L'écran reste ouvert HOME relâché (les deux mains libres). Une ligne par famille F1–F9 : toute la ligne rouge = famille masquée, « F2 » en bleu = en partie, en vert = tout proposé ; un carré par son ; sur la ligne choisie, le nom du son choisi (et sous le titre sa position « 3/5 », sa famille et ON / HIDDEN). Le nom de chaque famille prend sa couleur. SELECT la ligne, PRESET le son, SEL proposé (vert) / masqué (rouge), OCT− / OCT+ toute la famille masquée / proposée. Dernière ligne **→ NEXT PAGE** (l'aide dit « <- PRESET -> ») : PRESET ou OCT passe à **MAP CONTROL** (OCT− : **OPTION CONTROL**), qui ont la même dernière ligne ; une liste plus longue que l'écran a une barre de défilement à droite. N'importe quel bouton de menu (HOME, FX, ARP, SEQ, GLO, SAVE) ferme et enregistre. Un son masqué continue de jouer là où il est ; PRESET et les sauts de famille le sautent |
| **FX** + touche blanche 14, FX relâché d'abord | la molette de pitch : **BEND** ↔ **PAN** (gardé). En PAN, la molette envoie la piste choisie à gauche (vers le haut) ou à droite (vers le bas), au centre au repos |
| **vélocité des touches** | courbée : le jeu doux a plus de nuances (moitié linéaire, moitié racine carrée) |
| **molette de pitch** | par défaut ±2 demi-tons sur la piste sélectionnée (au repos : rien) ; réglable dans MAP CONTROL |
| **molette de modulation** | par défaut **MORPH** entre les deux dernières scènes rappelées (sinon aftertouch) ; réglable |
| **pédale** | tient les notes (sustain) |
| K1–K4 | la page de boutons affichée (FX change de page : SOUND, MACRO, LIVE, SETUP, USER, **EQ** (la sortie : BASS, MID, TREBLE, MID FREQ), DRUM EQ ; ordre et pages affichées : MAP CONTROL) |
| **SHIFT + K1–K4** | la page LIVE (MORPH, SWING, PUMP, LEVEL), sans quitter la page affichée |
| SELECT / PAGE / PRESETS | piste / tempo / son (comme SELECT, ALGO, PRESET sur le FM-1) |
| **MASTER** (8e encodeur) | volume général |
| ENV | UNDO de la piste sélectionnée ; **SHIFT + ENV** : REDO (rien sur A : que du live) |
| **SHIFT + SELECT** (ou ARP + SELECT) | **SOUND EDIT** : tous les paramètres du son, LFO calés sur le tempo, matrice de modulation (voir le manuel) |
| SELECT (tenu : couche des pistes — ses touches noires comme celles du FM-1 : 1re (Do#3) MUET LES 4, 4e (Sol#3) A VOL ; appuyé : valide un son en attente ; tenu + molette SELECT : pointer une ligne, depuis A : échange), SEQ (tenu : couche des effets), FX (pages des boutons) | comme sur le FM-1 |
| HOME, SAVE, ARP, GLO, PLAY, REC, EDIT | comme sur le FM-1 ([05](https://github.com/Tom-Rfid/OpenVALive_FM-1/blob/dev/docs/_FR/05-utilisation-fm1.md)) |
| **Bluetooth** | tapé : TAP tempo ; **tenu une demi-seconde : MAP CONTROL** |
| **MIDI OUT** (prise TRS) | horloge, START/STOP et notes : tout ce qui sort en USB sort aussi là |

### MAP CONTROL et OPTION CONTROL : les pages, ce que font les faders, les molettes et les boutons

**Tiens le bouton Bluetooth** (ou GLO + SEL) : **MAP CONTROL** s'ouvre. D'abord les pages de boutons dans l'ordre de
FX (PRESETS en déplace une, PAGE l'affiche / la masque), puis les boutons de MACRO, LIVE, EQ et USER (K1 à K4
chacune), FADER 1 à 4 (la banque USER) et les molettes PITCH et MOD. **HOME + touche blanche 7 (Si3)** ouvre **OPTION
CONTROL** : DRUM KEYS (ORIGINAL / GM), **PERC 1–6** (chaque touche ré# / sol# du clavier de batterie, de Ré#3 à Sol#5,
sa propre percussion), KEYS ON SCREEN, et pour T1–T4 et A la page qu'amène un nouveau son (SAME PAGE : aucune). Sur une
ligne **EDIT** remet sa valeur par défaut ; la dernière ligne, MENU, passe à SOUNDS / MAP CONTROL / OPTION CONTROL
avec PRESETS ou OCT. Gardé à la fermeture, sans SAVE.

| Geste dans la page | Effet |
|---|---|
| bouger un fader, une molette, tourner K1–K4 | sa ligne est choisie (et le fader, la molette continuent de jouer) |
| SELECT | ligne précédente / suivante |
| PRESETS | la cible : LEVEL, ENERGY, SPACE, TENSION, DENSITY, MORPH, SWING, PUMP, **FILTER, RESO, DRIVE, DELAY, CHORUS, CRUSH, DECAY**, SOUND (un bouton du son de la piste), TEMPO (60–180), BEND, AFTER (aftertouch), **MIDI CC**, BASS, MID, TREBLE, MID FREQ (l'EQ de sortie), **PAN** (une piste à gauche / droite, cran au milieu), -- (rien) |
| PAGE | l'option : la piste (1–4, ou la piste sélectionnée), ALL (toutes les pistes, pour les effets), le bouton du son, le numéro de CC (0–127, par exemple pour piloter la T-8) |
| EDIT | la ligne revient à sa valeur par défaut |
| tape sur Bluetooth, ou un bouton de menu (HOME, FX, ARP, SEQ, GLO, SAVE), ou SEL | ferme la page ; la carte est gardée dans la flash (et avec SAVE) |

Les **effets** tiennent leur valeur sur les pistes choisies tant que la commande est levée ; **tout en bas** (sous
2 %), les sons reviennent tels qu'ils sont enregistrés. Réglages d'usine : K1–K4 = FILTER, RESO, DELAY, CHORUS sur
la piste sélectionnée ; faders USER = FILTER, DELAY, DRIVE, CRUSH sur tout le mix ; PITCH = BEND ; MOD = MORPH.

Couleurs des pads : effets **bleu** (boucles), **orange** (filtre, delay, drive, crush), **rouge** (breaks), blanc
(tout arrêter) ; allumé fort = en cours, moyen = verrouillé. GLO : scènes mémorisées en bleu, la dernière plus
claire. SELECT : pistes en vert (rouge = coupée). ARP : violet. SAVE : sons gardés en ambre. DRUMS : rouge, le pad s'allume quand on
frappe. CLIPS : selon le son (rouge batterie, vert acid, rose USER, bleu les autres), vif en lecture, rouge qui clignote
pendant la prise. MIDI : blanc.

## A, la ligne live

Tourne SELECT après la piste 4 : la ligne **A** sous les pistes passe en menthe. Les touches jouent alors le son propre
de A, en direct par-dessus les boucles, sans l'enregistrer (PRESETS le choisit parmi les sons de synthé). Tape **ARP**
pour un arpège des touches tenues : UP, DOWN, UP/DOWN, RANDOM, OFF. Avec A sélectionnée, les effets de jeu, CLEAR
et le Creative Engine agissent sur toutes les pistes (pas d'UNDO / REDO sur A : que du live) ; avec une piste sélectionnée, sur elle seule.
SELECT + 5e pad coupe les quatre pistes (A continue) et quitte A ; SELECT + 6e pad = CLEAR. Une boucle relâchée joue
jusqu'à la fin de sa tranche, puis la musique reprend en rythme.

## Synchroniser la Roland T-8

1. Câble **TRS-MIDI type A** de la prise MIDI OUT du SMK-37 à l'entrée MIDI de la T-8 (la T-8 est en type A).
2. Sur la T-8 : *MIDI Clock Sync* en **AUTO** (ou **MIDI**).
3. Sur le SMK-37 : **SELECT + 9e pad** (ou SELECT + 9e touche blanche, Ré4) : CLOCK OUT allumé.
4. PLAY : la T-8 démarre avec OpenVALive, au même tempo ; STOP l'arrête.

Les notes de la piste 2 sortent sur le canal 2 : la basse de la T-8 les joue si elle écoute ce canal.

## Plan de test (l'Elite d'abord)

Note pour chaque ligne : ✅, ❌ (et ce qui se passe), ou ? (pas compris).

| # | Geste | Attendu |
|---|---|---|
| **0** | **Installation** | |
| 0.1 | `fm1_install.py --port SMK --info` avant | l'identité actuelle (`SMK-37 Elite_…`) |
| 0.2 | installer le paquet Elite | « done », le clavier redémarre |
| 0.3 | `--info` après | `SMK-37 Elite_8000300` |
| **1** | **Démarrage** | |
| 1.1 | allumer | « OPENVALIVE », puis l'écran du FM-1 (en-tête, oscilloscope, 4 pistes, boutons) |
| 1.2 | jouer une touche | du son (casque / sortie), l'oscilloscope bouge |
| 1.3 | éteindre, rallumer en tenant HOME | « SAFE START » |
| **2** | **Commandes** | |
| 2.1 | chaque touche, doux puis fort | toutes jouent ; la vélocité change le son |
| 2.2 | OCT− / OCT+ | l'octave change (affichée) |
| 2.3 | pads en PERFORM : tenir le pad 1 en lecture | boucle d'une mesure tant que tenu ; pad bleu vif |
| 2.4 | taper le pad 6 | FILTRE FERMÉ verrouillé ; le 2e pad en partant de la droite en bas (ALL OFF) l'arrête |
| 2.5 | tenir le 1er pad du bas (DELAY) et appuyer de plus en plus | le delay monte avec la pression |
| 2.6 | MODE (en haut à droite) jusqu'à DRUMS (rouge) ; tenir REC, pad 2, lâcher REC | les pads jouent leur kit, grosse caisse en bas à gauche ; REC tenu : 5 pads rouges ; le kit 2 est pris |
| 2.6b | sur la batterie, tenir un pad en appuyant de plus en plus fort, puis relâcher la pression | roulement 1/8, puis 1/16, puis 1/32 (affiché ROLL) ; il s'arrête |
| 2.6c | MODE jusqu'à SCÈNES (bleu) : REC + pad 2, REC lâché en premier, puis pad 2, puis le 2e pad du bas deux fois | scène 2 enregistrée (titre vert), rappelée (pad bleu vif) ; piste 1 coupée puis remise |
| 2.7 | MODE jusqu'à CLIPS (menthe) : REC + pad 1, REC lâché en premier ; jouer quelques notes ; pad 1 | le pad clignote rouge, la ligne se remplit de vert ; pad 1 : le clip joue une fois (SINGLE) ; MODE + pad 1 : LOOP sur le temps (vert / violet), une tape le libère |
| 2.7b | MODE jusqu'à MIDI (blanc), un pad | une note sur le canal 16 (DAW / MIDI OUT), aucun son interne |
| 2.8 | faders 1–4 | volume de chaque piste (message LEVEL n) |
| 2.9 | FADER BANK, puis faders | ENERGY, SPACE, TENSION, DENSITY ; voyant allumé |
| 2.9b | FADER BANK encore (USER, voyant qui clignote), fader 2 monté puis tout en bas | le delay arrive sur tout le mix, puis s'en va |
| 2.9c | tenir Bluetooth : MAP CONTROL ; bouger le fader 1, tourner PRESETS jusqu'à MIDI CC, PAGE sur 74, taper Bluetooth | la ligne FADER 1 est choisie ; fermé ; le fader 1 (banque USER) envoie le CC 74 |
| 2.10 | molette de pitch | la note se plie, revient au centre |
| 2.11 | 2 scènes rappelées, molette de modulation | MORPH glisse de l'une à l'autre |
| 2.12 | pédale | les notes tiennent jusqu'au relâchement |
| 2.13 | les 8 encodeurs | K1–K4 la page, SELECT piste, PAGE tempo, PRESETS son, MASTER volume |
| 2.14 | SHIFT + K3 | PUMP |
| 2.15 | ENV, puis SHIFT + ENV | UNDO, REDO |
| 2.16 | Bluetooth tapé 4 fois | TAP, le tempo suit (tenu : MAP CONTROL) |
| 2.17 | voyants des boutons | PLAY allumé en lecture, REC en enregistrement, SAVE quand il y a à sauver |
| **3** | **Live** | |
| 3.1 | REC, jouer, REC | boucle enregistrée |
| 3.2 | jouer librement, EDIT | CAPTURE : ce qui vient d'être joué devient la boucle |
| 3.3 | GLO + pads 9 et 10, puis GLO + pads 1 et 2 | scènes mémorisées puis rappelées à la mesure suivante |
| 3.4 | GLO + PLAY | la chaîne de scènes |
| 3.5 | ARP + pads | Creative Engine (MUTATE, FILL…) |
| **4** | **MIDI OUT et T-8** | |
| 4.1 | brancher la T-8, SELECT + pad 9, PLAY | la T-8 démarre en même temps, au même tempo |
| 4.2 | changer le tempo (PAGE) | la T-8 suit |
| 4.3 | STOP | la T-8 s'arrête |
| **5** | **Sauvegarde** | |
| 5.1 | PARA (SAVE) + Do#4 (SAVE ALL), PARA lâché en premier | « SAVING », PARA clignote, puis « SAVED » en vert ; PARA seul ne sauve rien |
| 5.2 | éteindre, rallumer | le set revient (boucles, sons, scènes) |
| **6** | **Ordinateur** | |
| 6.1 | USB : un DAW voit le port MIDI | notes et horloge sortent ; une note entrante joue la piste |
| 6.2 | audio USB | le son du clavier arrive dans le DAW ; le son de l'ordinateur sort du clavier |
| **7** | **Charge** | |
| 7.1 | 4 pistes en boucle, beaucoup de notes | pas de craquement ; le CPU affiché reste sous 80 % |
| **8** | **Retour (si besoin)** | |
| 8.1 | atelier SMK37 OPEN → *Switch to Open firmware* ou *Restore factory V16* | le firmware de M-VAVE revient |

À me renvoyer : les lignes ❌ avec ce qui se passe, une photo de l'écran au démarrage, la sortie de `--info`.

**Pour le Pro** : le même plan avec le paquet `-pro-`. Le Pro et l'Elite ont la même carte et le même scanner dans
le kit de M-VAVE ; les différences de modèle sont gérées par le SDK.

## Construire soi-même

```bash
bash ~/fm1-base/tools/get_toolchain.sh                    # la toolchain JieLi Linux (JIELI_TOOLCHAIN)
git clone https://github.com/m-vave/smk37_open ~/smk37_open
git -C ~/smk37_open checkout $(cat targets/smk37/SMK37_OPEN_COMMIT)
python3 targets/smk37/build.py --model elite --smk37-open ~/smk37_open --fm1-base ~/fm1-base
python3 targets/smk37/build.py --test --smk37-open ~/smk37_open --fm1-base ~/fm1-base   # sans toolchain
```

Le script fait ce que `sdk/Build.ps1` de M-VAVE fait sous Windows : compile nos fichiers et le scanner de M-VAVE,
lie avec le SDK, et emballe avec le chargeur de mise à jour du SDK (écrivain de paquet porté de `Package.cs`, MIT).
Il vérifie que le tas du SDK garde ses 48 Kio (en listant sinon ce qui prend la RAM) et que les interruptions et le scan du panneau sont en RAM interne. L'écrivain de paquet donne des fichiers identiques octet pour octet à `Package.cs` de M-VAVE (vérifié avec mono sur une image d'essai, 17 nov. 2026).

Comment c'est fait : [targets/smk37/README.md](../../targets/smk37/README.md).
