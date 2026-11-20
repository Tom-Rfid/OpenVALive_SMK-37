# 21 — FM-1 ↔ SMK-37 : les commandes côte à côte

*(Copie de `docs/_FR/19-commandes-fm1-smk37.md` d'OpenVALive CORE, la même dans OpenVALive_FM-1 et OpenVALive_SMK-37.)*

L'app commune (`app/`) est écrite pour le panneau du FM-1 ; le SMK-37 y branche ses commandes
(`targets/smk37/smk37_app.c` dans OpenVALive_SMK-37 : `BUTTON_MAP`, `ENCODER_MAP`, `smk_screen_key`, les pads).
**Règle :** un geste ajouté à l'app est ajouté ici, et vérifié sur les deux (script du simulateur FM-1, test hôte
SMK-37). Même fonctionnement sur les deux ; le SMK-37 ajoute seulement ce qu'il a en plus (pads, faders, molettes,
pédale, plus de touches).

## Boutons (mêmes noms sur les deux panneaux)

| Bouton de l'app | FM-1 | SMK-37 | Seul | Tenu (couche) |
|---|---|---|---|---|
| `B_FX` (`BTN_PAGES`) | FX | FX | page de boutons suivante (ordre de MAP CONTROL) | remise à zéro de la ligne (touche 1), molette BEND / PAN (14), carte batterie (15 / 16) |
| `B_SEL` (`BTN_TRACKS`, `BTN_VALIDATE`) | SEL | NOTE REPEAT | prend un son en attente | couche pistes : mutes (tout de suite), puis CLEAR, QUANTIZE, CLOCK, EUCLID, CONTROLLER (validés au relâché) ; touches noires MUTE ALL, A VOL ; clignote vite quand un son l'attend (+SEL) |
| `B_SEQ` (`BTN_PERFORM`) | SEQ | SEQ | tapé : la boucle se relâche | couche jeu (16 effets), titre « ∞ DIV / FILTER » |
| `B_ARP` | ARP | ARP | sur A : mode d'arpège | couche créative |
| `B_GLO` | GLO | BT | — | couche scènes : 1–8 LOAD, 9–16 SAVE (roses) ; GLO + SEL : MAP CONTROL ; GLO + PLAY : chaîne |
| `B_HOME` | HOME | PATCH | panic | PANIC / RESTART / STOP, OPTION CONTROL (touche blanche 7), SOUNDS (touche blanche 8), LOOP EDIT (4e touche noire) : validés au relâché |
| `B_SAVE` | SAVE | PARA | — (seul : rien) | validé au relâché : touches blanches 1–8 LOAD USER 1–8, 9–16 SAVE en USER 1–8, 1re touche noire **SAVE ALL** (le live set ; clignote quand il y a quelque chose à sauver) |
| `B_ENV` / `B_LFO` | ENV / LFO | SCALE / CHORD | UNDO / REDO | — |
| `B_EDIT` | EDIT | GLOBE | CAPTURE (ENV l'annule) ; dans un menu : valeur par défaut de la ligne ; clignote pendant l'enregistrement | EDIT + note : punch ; LOOP EDIT : EDIT + touches = accord |
| `B_PLAY` / `B_REC` | PLAY / REC | PLAY / REC | transport / enregistrement (REC et CAPTURE seulement en lecture) | SEL + PLAY tap tempo, SEL + REC métronome ; REC + touche blanche 1 (validé au relâché) : RESET de la piste |
| `B_OCTDN` / `B_OCTUP` | OCT− / OCT+ | OCT− / OCT+ | octave ; saut de famille ; boucle ×2 / ÷2 | — |
| — | — | KNOB BANK = SHIFT | — | SHIFT + K1–K4 : page LIVE ; SHIFT + SELECT : SOUND EDIT ; SHIFT + pad (SCÈNES) : mémoriser la scène ; SHIFT + pad (CLIPS) : effacer le clip |
| — | — | FADER BANK | faders : volumes → macros → USER | — |
| — | — | < (Bluetooth) | TAP tempo | tenu 0,5 s : MAP CONTROL |
| — | — | > (PAD/STEP) | pads : CLIPS → DRUMS → PERFORM → SCÈNES → MIDI | — |
| — | — | pad MODE (en haut à droite) | pareil : une tape = le mode suivant (sa couleur) | MODE + pad (CLIPS) : LOOP (verrouillé, depuis le temps) ; une tape seule : SINGLE |
| — | — | pad REC (en bas à droite) | — | REC + pad, REC lâché en premier : enregistrer un clip / sauver une scène / choisir le kit des pads |

## Boutons rotatifs

| Encodeur de l'app | FM-1 | SMK-37 |
|---|---|---|
| `EN_SELECT` | SELECT | encodeur 1 |
| `EN_PRESET` | PRESET | encodeur 2 (PRESETS) |
| `EN_K1`..`EN_K4` | K1–K4 | encodeurs 3–6 |
| `EN_ALGO` | ALGO (tempo) | encodeur 7 (PAGE) |
| master | potentiomètre MASTER | encodeur 8 |

## Touches sous une couche

| | FM-1 (27 touches, Fa3–Sol5) | SMK-37 (37 touches, Do3–Do6) |
|---|---|---|
| emplacements 1–16 | les 16 touches blanches Fa3 … Sol5 | les 16 premières blanches Do3 … Ré5 |
| touches noires (`BLACK_KEY[]`) | Fa#3 Sol#3 La#3 Do#4 Ré#4 Fa#4 … (11) | Do#3 Ré#3 Fa#3 Sol#3 La#3 … (la n-ième noire = la n-ième du FM-1) |
| SEL + 1re noire : MUTE ALL | Fa#3 | Do#3 (et 13e pad de jeu en SCÈNES) |
| SEL + 4e noire : A VOL (validé au relâché) | Do#4 | Sol#3 (et 14e pad de jeu en SCÈNES) |
| touches PERC de batterie (OPTION CONTROL) | PERC 1–4 : Sol#3 Ré#4 Sol#4 Ré#5 | PERC 1–6 : Ré#3 Sol#3 Ré#4 Sol#4 Ré#5 Sol#5 (`APP_PERC_FIRST` / `APP_PERC_COUNT`) |
| lumières des touches | les LED des touches | le clavier à l'écran (KEYS ON SCREEN), couche tenue |

## Réglages (OPTION CONTROL, les deux)

| Ligne | Valeurs | Rôle |
|---|---|---|
| PLAY MODE | **LIVE** (par défaut) / FULL MIDI | LIVE : les touches et les pads jouent les sons internes (et sortent en MIDI). FULL MIDI : ils n'envoient que du MIDI (local off), pour un DAW ou un autre synthé ; aussi SEL + 16e touche blanche |
| DRUM KEYS | ORIGINAL / GM | la carte des touches de batterie |
| PERC 1–4 (FM-1) / 1–6 (SMK-37) | 10 percussions | ce que joue chaque Ré# / Sol# de la batterie |
| KEYS ON SCREEN | ON / OFF | le clavier à l'écran quand une couche est tenue |
| T1–T4, A NEW SOUND | SAME PAGE / une page | la page de boutons qu'amène un nouveau son |

## Voyants

Boutons : **FX** et **SEL** allumés ; disponible mais pas actif = faible, actif = allumé, quelques-uns clignotent vite (MUTE ALL…), rien ne respire ;
**EDIT** allumé en lecture (clignote pendant l'enregistrement) ; **SAVE** allumé quand il y a quelque
chose à sauver (un mute ne compte pas), clignote pendant la sauvegarde (par étapes sur environ une seconde, chaque
partie seulement si elle a changé).
Clavier à l'écran (et les touches du FM-1 quand elles le peuvent) : Couleurs : **jaune** appuyée ; **violet** quelque chose d'actif (une piste qui joue, un effet en cours…) ; **menthe** disponible, pas actif ; **rose** validé au relâché (lâcher le bouton en premier pour le prendre) ; **menthe / rose en alternance** validé au relâché et actif (A VOL en cours…) ; **bleu** les écrans de réglage (SOUNDS, LOOP EDIT, OPTIONS) ; **rouge** une piste coupée (SEL).

## Seulement sur le SMK-37 (ses commandes en plus, rien dont le FM-1 a besoin)

| SMK-37 | Rôle | Sur le FM-1 |
|---|---|---|
| 16 pads : pads de réglage MODE + REC, 14 pads de jeu | CLIPS (SINGLE / LOOP), DRUMS (leur propre kit, MIDI canal 10, roulements à la pression), PERFORM (14 effets), SCÈNES (scènes, mutes, MUTE ALL, A VOL), MIDI (canal 16) | PERFORM, SCÈNES, mutes, MUTE ALL, A VOL : pareil sur les touches sous une couche. **Les CLIPS, les pads MIDI et les roulements n'ont pas d'équivalent FM-1** |
| 4 faders | volumes, macros, emplacements USER (FADER BANK) | les boutons rotatifs (pages LIVE / MACRO, MAP CONTROL) |
| molette de pitch, de modulation | BEND ou PAN, MORPH (réglables) | — |
| pédale de sustain | tient les notes | — |
| MIDI OUT (jack TRS) | notes, horloge | MIDI USB seulement |
| 10 touches de plus, courbe de vélocité | Do3–Do6 | 27 touches Fa3–Sol5 |
| SHIFT (KNOB BANK) | SHIFT + K1–K4 : page LIVE ; SHIFT + SELECT : SOUND EDIT | ARP + SEL : SOUND EDIT (aussi sur le SMK-37) ; page LIVE : pages FX |

## Seulement sur le FM-1

| FM-1 | Sur le SMK-37 |
|---|---|
| une LED dans chaque touche (les emplacements de la couche allumés) | le clavier à l'écran (KEYS ON SCREEN) |
| le haut-parleur intégré | casque / sortie ligne |
| potentiomètre MASTER | encodeur 8 |

**Règle pour la suite :** les fonctions communes sont ce que le FM-1 sait faire (ses 14 boutons, 27 touches, 6
boutons rotatifs). Un nouveau geste trouve d'abord sa place sur le FM-1 ; s'il ne peut pas (il lui faut un pad, un
fader, une pédale), il est seulement sur le SMK-37 et va dans le tableau ci-dessus.
