@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
if errorlevel 1 echo Instalace se nezdarila. Zkontroluj chybu vyse; pri odeprenem pristupu spust jako spravce.
pause
