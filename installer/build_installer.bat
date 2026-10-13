@echo off
setlocal
rem ===========================================================================
rem  Baut das "echo"-Setup mit dem Inno Setup Compiler (ISCC).
rem  Voraussetzung: das Plugin wurde bereits gebaut
rem  (Builds\VisualStudio2026\x64\Release\...), sonst fehlen die Quelldateien.
rem ===========================================================================

set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"

if not exist "%ISCC%" (
    echo.
    echo FEHLER: ISCC.exe nicht gefunden unter
    echo         "%ISCC%"
    echo Bitte den Pfad in dieser Datei anpassen.
    exit /b 1
)

echo Baue Setup aus "%~dp0echo.iss" ...
"%ISCC%" "%~dp0echo.iss"
if errorlevel 1 (
    echo.
    echo FEHLER: Build fehlgeschlagen.
    exit /b 1
)

echo.
echo Fertig. Das Setup liegt in "%~dp0output".
endlocal
