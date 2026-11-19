@echo off
rem OpenVALive - installs the firmware on the SMK-37 (double-click)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0flash-smk37.ps1" %*
