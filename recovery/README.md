# Recovery — M-VAVE SMK-37 / FM-1 that no longer boots

**EN** · [FR plus bas](#fr)

A firmware that crashes at start-up leaves the unit invisible over USB: no MIDI port, the M-VAVE workbench and
M-UPGRADE cannot see it. The JieLi chip inside still has a factory USB loader ("UBOOT", USB `4c4a:8057`,
`WL80UBOOT1.00`). It opens when the chip receives the JieLi **USB key** on its USB lines as it powers up.

These scripts send that key **from a Raspberry Pi's GPIO, through the USB cable: the unit stays closed.**
Then [M-Flasher](https://github.com/jvitkauskas/m-flasher) backs up the flash and writes a firmware (`.fwsc`)
without touching the bootloader, the key calibration or the Bluetooth pairings.

Tested once, on an SMK-37 Elite (Raspberry Pi 5): key, 1 MB backup, firmware written with M-Flasher, unit back.
The FM-1 uses the same JieLi family and M-Flasher supports it, but **the FM-1 path has not been tested yet**.

| File | What it is |
|---|---|
| `rescue-en.sh` / `rescue-fr.sh` | the menu script, English / French (Raspberry Pi 4 or 5, Raspberry Pi OS or Ubuntu) |
| `rp2040_usb_key.py` | the key alone, for a Raspberry Pi Pico (MicroPython) — **untested**, please report |

**What you need:** a Raspberry Pi 4 or 5, a USB-A → USB-C *data* cable, 3 wires (and two 100 Ω resistors if you have
them), a multimeter for one check.

**The cable:** open its jacket in the middle; on the green (D+), white (D-) and black (GND) wires add a wire to the
Pi: GPIO17 (pin 11), GPIO27 (pin 13), GND (pin 9). **Never the red (5 V) wire.** The USB-A end stays in the Pi:
after the key the Pi is the USB host that sees the chip. Menu 2 of the script draws it all.

```
chmod +x rescue-en.sh
./rescue-en.sh
```
Then follow menu 1: install (3) → wiring (2) → key (4: switch the unit OFF, Enter, switch it ON within 6 s) →
check (5: a `4c4a` device) → backup (6) → firmware (10) → flash (9, M-Flasher).

Keep your backup. Never restore another unit's backup onto yours (its calibration is per unit).
With thanks to [kagaimiq/jielie](https://kagaimiq.github.io/jielie/isp/usb/usb-key.html) for the USB key and to
M-Flasher's author.

---

## FR

Un firmware qui plante au démarrage rend l'appareil invisible en USB : plus de port MIDI, ni le workbench M-VAVE ni
M-UPGRADE ne le voient. La puce JieLi garde un chargeur USB d'usine (« UBOOT », USB `4c4a:8057`, `WL80UBOOT1.00`),
qui s'ouvre quand elle reçoit la **clé USB** JieLi sur ses lignes USB au moment où elle s'allume.

Ces scripts envoient cette clé **depuis le GPIO d'un Raspberry Pi, par le câble USB : on n'ouvre pas l'appareil.**
Ensuite [M-Flasher](https://github.com/jvitkauskas/m-flasher) sauvegarde la flash et écrit un firmware (`.fwsc`)
sans toucher au chargeur de démarrage, à la calibration des touches ni aux appairages Bluetooth.

Testé une fois, sur un SMK-37 Elite (Raspberry Pi 5) : clé, sauvegarde de 1 Mo, firmware écrit avec M-Flasher,
appareil revenu. Le FM-1 est de la même famille JieLi et M-Flasher le gère, mais **pas encore testé sur le FM-1**.

| Fichier | Ce que c'est |
|---|---|
| `rescue-fr.sh` / `rescue-en.sh` | le script à menu, en français / anglais (Raspberry Pi 4 ou 5, Raspberry Pi OS ou Ubuntu) |
| `rp2040_usb_key.py` | la clé seule, pour un Raspberry Pi Pico (MicroPython) — **pas testé**, merci de faire un retour |

**Il faut :** un Raspberry Pi 4 ou 5, un câble USB-A → USB-C *de données*, 3 fils (et deux résistances de 100 Ω si
tu en as), un multimètre pour une vérification.

**Le câble :** ouvre la gaine au milieu ; sur le vert (D+), le blanc (D-) et le noir (GND), ajoute un fil vers le Pi :
GPIO17 (broche 11), GPIO27 (broche 13), GND (broche 9). **Jamais le rouge (5 V).** Le bout USB-A reste dans le Pi :
après la clé, c'est lui l'hôte USB qui voit la puce. Le menu 2 du script dessine tout.

```
chmod +x rescue-fr.sh
./rescue-fr.sh
```
Puis suis le menu 1 : installation (3) → câblage (2) → clé (4 : interrupteur sur OFF, Entrée, interrupteur sur ON
dans les 6 s) → vérification (5 : un appareil `4c4a`) → sauvegarde (6) → firmware (10) → flash (9, M-Flasher).

Garde ta sauvegarde. Ne restaure jamais la sauvegarde d'un autre appareil sur le tien (la calibration est propre à
chaque appareil).
