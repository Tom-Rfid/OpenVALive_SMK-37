# 03 — Le synthé à l'intérieur : OpenVALive CORE, et comment c'est câblé

Ce dépôt contient la **moitié SMK-37** d'OpenVALive. Le synthé, le Live Core et les écrans de l'app sont ceux
d'[OpenVALive CORE](https://github.com/Tom-Rfid/OpenVALive_CORE) (MIT), inclus ici comme sous-module git dans `va/`. La même app tourne sur le FM-1
([OpenVALive_FM-1](https://github.com/Tom-Rfid/OpenVALive_FM-1)). Cette page est la version courte ; la complète est dans OpenVALive CORE
([architecture](https://github.com/Tom-Rfid/OpenVALive_CORE/blob/dev/docs/_FR/03-architecture-technique.md)).

## Les couches

```
 matériel SMK-37    37 touches, 16 pads RGB + pression, 4 faders, molettes, pédale, encodeurs, écran, MIDI OUT, flash
      │
 SDK de M-VAVE      smk37_open (GPL-3.0 + exception de liaison, bibliothèque binaire) : démarrage, USB, mise à jour, flash
      │                                                                      ── ce dépôt : targets/smk37/
 board/             le scanner de M-VAVE (inchangé), panel_ols.c (événements, écran, RGB), audio_ols.c (l'interruption)
 smk37_app.c        la plateforme qu'attend l'app (l'API plat.h de fm1-base) + les ajouts du SMK-37 + inclut l'app
 app.h              nom, splash, APP_TOUCH, APP_OVERLOAD, tests hôte ; inclut va/app/openvalive_app.h
      │                                                                      ── le sous-module : va/ (MIT)
 va/app             l'instrument : pages, couches, MAP CONTROL, SOUND EDIT, sauvegarde, MIDI   (une unité, -Oz)
 va/core/live       le Live Core : horloge, 4 pistes, REC, CAPTURE, undo, scènes, Morph, Creative Engine
 va/core/engines    VA (2 osc + sub + bruit, filtre, 2 env, 2 LFO, matrice de modulation, effets) · DRUM (famille 808)
 va/core/dsp        PolyBLEP, ZDF SVF, ADSR, LFO, delay, chorus, phaser    (va/app/ols_dsp.c, -O2)
```

## Une note, de la touche au haut-parleur

| Étape | Où | Quand |
|---|---|---|
| 1. Le scanner de M-VAVE lit une touche, sa vélocité selon la calibration d'usine (lue seulement) | `board/scan.c` (M-VAVE) | interruption de scan |
| 2. `panel_ols.c` transmet l'événement ; `smk37_app.c` le traduit : note, pad, fader, case de couche… | `board/panel_ols.c`, `smk37_app.c` | tâche de contrôle |
| 3. L'app pousse un `ols_event_t` dans la file sans verrou (l'interface ne touche jamais un moteur) | `va/app/src/app_events.c` | tâche de contrôle |
| 4. L'interruption audio demande 128 trames au rendu de l'app | `board/audio_ols.c` → `va/app/src/app_audio.c` | toutes les 2,9 ms |
| 5. Le Live Core joue les boucles à l'échantillon près et envoie les notes aux 4 moteurs | `va/core/live/live.c` | dans le rendu |
| 6. Chaque moteur rend ; modulation et calculs coûteux toutes les 32 échantillons | `va/core/engines/va/` | dans le rendu |
| 7. Sortie int32 Q23 stéréo, mixée avec l'audio USB ; le MIDI sort aussi par le jack TRS (une T-8) | `board/audio_ols.c` | dans le rendu |

## Ce qu'ajoute le SMK-37

| Quoi | Où |
|---|---|
| Pads PERFORM / PLAY, faders (niveaux, macros, USER), molettes, pédale, MASTER, bouton Bluetooth = TAP / MAP CONTROL | `smk37_app.c` |
| Deux crochets dans l'app commune : `APP_KEY_LAYER(k)` (pads comme touches de couche), `APP_MAP_SLOTS 10` (faders et molettes dans la MAP CONTROL) | `smk37_app.c`, avant d'inclure l'app |
| Stockage : 5 objets, 2 copies chacun avec CRC, dans les emplacements de projets d'Open (jamais la calibration ni l'appairage) | `smk37_app.c` |
| Un paquet par modèle (`SMK-37 Elite_8XXYYZZ` / `Pro`), identique à l'octet près à celui du `Package.cs` de M-VAVE | `build.py` |

## Changer le synthé

Un changement du son, du Live Core ou des écrans se fait **dans OpenVALive CORE**, s'y teste (et sur le
FM-1), puis ce dépôt déplace sa version fixée :

```bash
sh scripts/va.sh status            # quelle version est fixée
sh scripts/va.sh update            # va/ au dernier dev    (ou : use v0.4.0 · use <commit> pour revenir en arrière)
FM1_BASE=~/fm1-base SMK37_OPEN=~/smk37_open sh tests/run_tests.sh
git add va && git commit -m "va: v0.4.0"
```

Windows : `.\scripts\va.ps1 status | update | use v0.4.0 | reset`. Règles : [18](18-regles-git-et-publication.md).
