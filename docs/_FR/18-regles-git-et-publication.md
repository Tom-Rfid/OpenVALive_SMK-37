# 18 — Règles : code, git et publication

Les règles que tout le monde suit ici, humains et agents IA. Elles gardent OpenVALive rapide, lisible et publiable.
La version pour les agents, avec les commandes, est [CLAUDE.md](../../CLAUDE.md).

## Branches

| Branche | Ce qui y va |
|---|---|
| `dev` (par défaut) | le travail : chaque pull request vise `dev` |
| `feat/<sujet>`, `fix/<sujet>` | un sujet chacune, depuis `dev`, retour dans `dev` par pull request |
| `main` | **les releases seulement** : `dev` est fusionné dans `main` par une pull request « Release vX.Y.Z », puis publié |

Pas de push direct sur `main`. Un correctif urgent sur une release : `fix/<sujet>` depuis `main`, PR vers `main`,
puis `main` fusionné dans `dev`.

## Commits et pull requests

- Un sujet par commit ; un résumé court en anglais (impératif, ≤ 72 caractères), puis le *pourquoi* dans le corps.
- Dire **où** c'est testé : PC, simulateur, vrai appareil (modèle, firmware).
- `FM1_BASE=~/fm1-base SMK37_OPEN=~/smk37_open sh tests/run_tests.sh` doit finir par « All tests pass. » avant une PR ; la CI le relance.
- Un travail fait avec un agent IA le dit : une ligne `Co-Authored-By:` dans le commit.

## Le sous-module `va/`

- `va/` est [OpenVALive CORE](https://github.com/Tom-Rfid/OpenVALive_CORE) à **un commit fixé**. Un clone demande `git clone --recursive`, ou
  `sh scripts/va.sh init` ensuite.
- **Ne jamais committer dans `va/` depuis ici.** Un changement du synthé, du Live Core ou de l'app se fait dans
  OpenVALive CORE (branche depuis son `dev`, PR, tests), puis la version fixée bouge ici.
- Déplacer la version fixée est un commit à part : `sh scripts/va.sh update` (dernier `dev`) ou
  `sh scripts/va.sh use v0.4.0` (un tag ; aussi pour revenir en arrière), lancer les tests, puis
  `git add va && git commit -m "va: v0.4.0"`.
- **Une release fixe un tag** d'OpenVALive CORE, pas un commit mouvant : publier OpenVALive CORE d'abord.
- `sh scripts/va.sh status` dit si `va/` a bougé ou a des changements locaux ; `sh scripts/va.sh reset` annule.
  Windows : `.\scripts\va.ps1` avec les mêmes commandes.

## Code

- **C99, `float` seulement** (suffixe `f`), pas de `double` : le compilateur JieLi le refuse (`-Wdouble-promotion -Werror`).
- **Pas de bibliothèque C, pas de `malloc`** dans le cœur et l'app : `<stdint.h>` seulement ; les maths par `dsp_*`.
- Les calculs coûteux (tan, exp, pow) seulement au **rythme de contrôle** (toutes les 32 échantillons) ou quand un
  paramètre change.
- L'interruption audio ne bloque jamais : ni stockage, ni écran. L'interface ne touche jamais un moteur : elle envoie
  des événements.
- Les moteurs dorment en silence et rendent n'importe quelle taille de bloc à l'identique. **Une refonte garde le son
  identique au bit près.**
- Les nouveaux paramètres vont **à la fin** (les live sets sauvés les rangent par numéro).
- Petites fonctions avec un seul rôle, des commentaires qui disent *pourquoi*, l'anglais dans le code.
- Licence de ce dépôt : **GPL-3.0 (avec l'exception de liaison du SDK)**. Ne jamais copier de code dont la licence ne le permet pas ; le code MIT
  d'ailleurs garde sa notice.

## Docs

- **Anglais et français ensemble** : `docs/_EN` et `docs/_FR`, mêmes numéros, même contenu.
- Le dépôt décrit ce qui est **fait ou en cours** ; les idées pas encore décidées restent en dehors.
- Chaque « testé » dit où et comment. `STATUS.md` et `CHANGELOG.md` suivent le travail.

## Publier une release

1. `dev` est vert ; `VERSION`, `CHANGELOG.md` et `STATUS.md` sont à jour.
2. Pull request `dev` → `main` « Release vX.Y.Z », fusionnée.
3. Actions > build > Run workflow, branche `main`, « release » cochée : le tag `vX.Y.Z` et la release sont créés.
4. Tester la release sur le vrai appareil (installer, jouer, sauver, éteindre et rallumer, ressortir) et le noter
   dans `STATUS.md`.

## The Firmware Hub

Le hub n'a pas encore de vérification automatique pour le SMK-37 : Sven regarde le dépôt et la release lui-même.

| Règle | Ici |
|---|---|
| Notre numéro sur le hub | `FM-1_037` (la fiche du hub ; la version y a son propre champ) |
| Un paquet par modèle, au format d'identité de M-VAVE | `SMK-37 Elite_8XXYYZZ` / `SMK-37 Pro_8XXYYZZ` (XXYYZZ = la version, `build.py`) : le chargeur de mise à jour du clavier et `Package.cs` n'acceptent rien d'autre |
| Une release GitHub avec les paquets | `openvalive-smk37-elite-X.Y.Z.fwsc`, `-pro-`, et les zips Windows, depuis Run workflow sur `main` |
| Le chemin du retour | la V016 de M-VAVE, par l'atelier web SMK37 OPEN |
| `fmhub.json` à la racine, licence GPL-3.0 | `what`, `author`, `kinds`, `image`, `color` ; pas de ligne `sounds` (tout est synthétisé) |
| Testé sur un vrai SMK-37, depuis le firmware de M-VAVE, et ressorti | noté dans `STATUS.md` avec la date et le modèle |
| **Jamais** écrire la calibration (`0xC1000`, enregistrements VM 51 à 54) ni l'appairage Bluetooth | vérifié par `build.py --check` |
