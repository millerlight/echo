# echo - Installer (Inno Setup)

Diese Konfiguration baut ein Windows-Setup, das das **echo**-VST3-Plugin
installiert, den **Preset-Ordner anlegt** und die Presets hineinkopiert.

## Dateien

| Datei | Zweck |
|---|---|
| `echo.iss` | Inno-Setup-Skript (die eigentliche Konfiguration) |
| `build_installer.bat` | Baut das Setup per ISCC |
| `output\` | Ergebnis: `echo-Setup-<version>.exe` (wird beim Bauen erzeugt) |

## Bauen

1. **Plugin bauen** (sonst fehlen die Quelldateien):
   ```
   Builds\VisualStudio2026\echo.sln  ->  Konfiguration Release / Plattform x64
   ```
   Ergebnis u. a.:
   - `Builds\VisualStudio2026\x64\Release\VST3\echo.vst3`
   - `Builds\VisualStudio2026\x64\Release\Standalone Plugin\echo.exe`

2. **Setup bauen**:
   ```
   installer\build_installer.bat
   ```
   oder direkt:
   ```
   "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" installer\echo.iss
   ```

Ergebnis: `installer\output\echo-Setup-1.0.0.exe`

## Was installiert wird

| Komponente | Quelle | Ziel |
|---|---|---|
| VST3-Plugin (Pflicht) | `...\Release\VST3\echo.vst3\*` | `{commoncf}\VST3\echo.vst3` (i. d. R. `C:\Program Files\Common Files\VST3\echo.vst3`) |
| Presets (Pflicht) | `..\presets\*.echo` | `{userdocs}\echo presets` (i. d. R. `C:\Users\<Name>\Documents\echo presets`) |
| Standalone (optional) | `...\Release\Standalone Plugin\echo.exe` | `{autopf}\echo\echo.exe` |

Der Preset-Zielordner heißt bewusst **`echo presets`** - genau den Namen sucht
das Plugin (`userDocumentsDirectory()/echo presets`). Nur so findet das
installierte Plugin die Presets ohne Codeänderung.

## Presets - wichtige Details

- `onlyifdoesntexist`: bereits vorhandene (evtl. selbst geänderte) Presets
  werden **nicht überschrieben**.
- `uninsneveruninstall`: Bei einer **Deinstallation bleiben die Presets
  erhalten** (sie sind Benutzerdaten - das Plugin schreibt/löscht dort selbst).
- Der Projektordner `D:\VST-output\echo\presets` ist nur die **Quelle** für den
  Installer. Er wird nicht direkt vom Plugin benutzt.

## Anpassen

- **Version / Name / Herausgeber**: oben im `echo.iss` bei den `#define`s.
- **Icon**: eine `echo.ico` neben `echo.iss` legen und in `[Setup]` die Zeile
  `;SetupIconFile=echo.ico` einkommentieren.
- **AppId** in `[Setup]` niemals ändern (sonst klappt Update/Deinstallation
  nicht mehr).

## Hinweis zu Admin / Benutzerordner

Das Skript läuft mit `PrivilegesRequired=admin`, weil nach
`Program Files\Common Files\VST3` geschrieben wird. Inno warnt dabei, dass
zusätzlich ein Benutzerordner (`{userdocs}`) benutzt wird.

In der Praxis ist das ok, wenn der Benutzer sich selbst per UAC erhöht (dann
ist `{userdocs}` = sein eigener Dokumente-Ordner). Problematisch wäre nur eine
Installation, bei der ein **anderer** Admin-Account per UAC-Übernahme
eingegeben wird - dann zeigt `{userdocs}` auf dessen Dokumente.

Möchte man das komplett vermeiden, gibt es zwei Wege:

1. **Pro Benutzer installieren** (kein Admin): im `echo.iss`
   - `PrivilegesRequired=lowest`
   - VST3-Ziel auf `{localappdata}\Programs\Common\VST3\echo.vst3`
   - Standalone auf `{localappdata}\Programs\echo`
   Die meisten modernen DAWs scannen den per-User-VST3-Ordner mit.
2. **Zwei getrennte Installer** (Admin für das Plugin, pro Benutzer für die
   Presets) - aufwändiger.
