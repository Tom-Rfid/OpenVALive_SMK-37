# OpenVALive - installs the firmware on an SMK-37 Elite or Pro plugged in over USB (Windows)
#
# Double-click FLASH-SMK37.bat, or: powershell -ExecutionPolicy Bypass -File flash-smk37.ps1
# The folder must hold: openvalive-smk37-elite-X.Y.Z.fwsc (or -pro-) and its .build_id, smk37_install.py,
# fm1_install.py and fm1ports.py.
# The package is checked by the keyboard before anything is written, and it carries M-VAVE's own update loader;
# the keys' and pads' calibration is not touched. Way back: M-VAVE's SMK37 OPEN workbench (Restore factory V16).
#   -PrepareOnly: only prepare Python 3.12 + mido + python-rtmidi
param([switch]$PrepareOnly, [string]$Package = "")
$ErrorActionPreference = "Stop"
function Ok($t) { Write-Host "    OK $t" -ForegroundColor Green }

# Python 3.12: python-rtmidi has no prebuilt package for very recent Pythons
# (3.14, 3.15) and building it needs Visual Studio. 3.12 has it ready.
if (-not (Get-Command py -ErrorAction SilentlyContinue)) {
    Write-Host "    The 'py' launcher is missing: installing the Python install manager (winget)."
    winget install --id Python.PythonInstallManager -e --accept-source-agreements --accept-package-agreements
    $env:Path = [Environment]::GetEnvironmentVariable("Path", "User") + ";" + [Environment]::GetEnvironmentVariable("Path", "Machine")
}
& py -3.12 --version *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host "    Installing Python 3.12..."
    & py install 3.12
    if ($LASTEXITCODE -ne 0) { winget install --id Python.Python.3.12 -e --accept-source-agreements --accept-package-agreements }
}
& py -3.12 -c "import mido, rtmidi" *> $null
if ($LASTEXITCODE -ne 0) {
    & py -3.12 -m pip install --user --disable-pip-version-check -q mido python-rtmidi
    if ($LASTEXITCODE -ne 0) { Write-Host "pip install mido python-rtmidi failed" -ForegroundColor Red; exit 1 }
}
Ok "Python 3.12 + mido + python-rtmidi"
if ($PrepareOnly) { exit 0 }

Set-Location $PSScriptRoot
if (-not $Package) { $Package = (Get-ChildItem $PSScriptRoot -Filter "openvalive-smk37-*.fwsc" | Select-Object -First 1).FullName }
if (-not $Package -or -not (Test-Path $Package)) { Write-Host "No openvalive-smk37-*.fwsc next to this script." -ForegroundColor Red; exit 1 }
$model = if ($Package -match "-pro-") { "Pro" } else { "Elite" }

Write-Host ""
Write-Host "OpenVALive for the SMK-37 $model" -ForegroundColor Cyan
Write-Host "Before installing:" -ForegroundColor Yellow
Write-Host "  - this package is for the SMK-37 $model only (the keyboard refuses the other model's)"
Write-Host "  - plug the SMK-37 in over USB (a DATA cable) and switch it on"
Write-Host "  - close the M-VAVE workbench, your DAW and every MIDI program"
Write-Host "  - back up your M-VAVE Open projects and samples with the workbench first"
Write-Host "  - way back: open.m-vave.com/smk37-workbench > Restore factory V16, or Switch to Open firmware"
Read-Host "Press Enter when ready" | Out-Null

# the keyboard's MIDI port is named "SMK-37 ..." by M-VAVE's firmware, "USB-Midi ..." by others (OpenVALive's too)
$port = $null
foreach ($name in @("SMK", "USB-Midi")) {
    & py -3.12 smk37_install.py --port $name --info
    if ($LASTEXITCODE -eq 0) { $port = $name; break }
}
if (-not $port) {
    Write-Host "SMK-37 not found: check the cable (data), that it is on, and that no program uses its MIDI port." -ForegroundColor Red
    Write-Host "It no longer starts at all? See https://github.com/Tom-Rfid/jieli-usbkey-unbrick" -ForegroundColor Yellow
    Read-Host "Press Enter to close" | Out-Null; exit 1
}
& py -3.12 smk37_install.py --port $port --force --no-resources $Package
Read-Host "Press Enter to close" | Out-Null
