@echo off
rem OpenVALive - installs OpenVALive on an SMK-37 in UPDATE MODE, with M-Flasher (Windows).
rem
rem   1. Put this file next to M-Flasher.exe (github.com/jvitkauskas/m-flasher), with the package
rem      openvalive-smk37-elite-X.Y.Z.fwsc (or -pro-) from the same zip.
rem   2. Switch the SMK-37 on holding SEQ + REC: "UPDATE MODE" on the screen (only firmwares that have it;
rem      Windows then sees a disk "WL82 UBOOT1.00": do not format it).
rem   3. Double-click this file.
rem Afterwards OpenVALive updates over MIDI: FLASH-SMK37.bat (keep it: it also installs another firmware from
rem OpenVALive, or from any firmware without SEQ + REC).
cd /d "%~dp0"
if not exist "M-Flasher.exe" (
    echo M-Flasher.exe is not in this folder: put MFLASHER-SMK37.bat next to it.
    pause
    exit /b 1
)
set "FW="
for %%f in (openvalive-smk37-*.fwsc) do set "FW=%%f"
if "%FW%"=="" (
    echo No openvalive-smk37-*.fwsc next to M-Flasher.exe: copy it from the OpenVALive zip.
    pause
    exit /b 1
)
echo Installs %FW% with M-Flasher on the SMK-37 in UPDATE MODE (SEQ + REC held at power-on).
echo The calibration and your data outside the firmware are kept; M-Flasher saves a backup first.
M-Flasher.exe firmware "%FW%" --board smk37 --restart --yes
pause
