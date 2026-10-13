; ============================================================================
;  Inno Setup Skript fuer das "echo"-Plugin
;  ---------------------------------------------------------------------------
;  Installiert:
;    * VST3-Plugin   -> {commoncf}\VST3\echo.vst3        (fuer die DAW)
;    * Standalone    -> {autopf}\echo\echo.exe           (optional)
;    * Presets       -> {userdocs}\echo presets           (hier sucht das Plugin)
;
;  Bauen:   installer\build_installer.bat   (oder direkt: ISCC echo.iss)
;  Ergebnis: installer\output\echo-Setup-<version>.exe
;
;  Wichtig: Die Presets liegen im Dokumente-Ordner des Benutzers, weil das
;  Plugin dort sucht und dort auch speichert/loescht (Schreibrechte!). Der
;  Ordner "..\presets" im Projekt dient nur als Quelle fuer diese Installation.
; ============================================================================

#define AppName       "echo"
#define AppVersion    "1.0.0"
#define AppPublisher  "millerlight"
#define AppURL        "www.millerlight.com"
#define AppExeName    "echo.exe"

; Quellen (Pfade relativ zu dieser .iss-Datei)
#define PluginBundle  "..\Builds\VisualStudio2026\x64\Release\VST3\echo.vst3"
#define StandaloneExe "..\Builds\VisualStudio2026\x64\Release\Standalone Plugin\echo.exe"
#define PresetSource  "..\presets"

; Zielordner der Presets - genau der Ordner, den das Plugin benutzt
#define PresetDir     "{userdocs}\echo presets"

[Setup]
; AppId niemals aendern, sonst erkennt Windows Updates/Deinstallation nicht.
AppId={{06945E97-780F-4D6E-B5B5-43547027A1AB}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}
VersionInfoVersion=1.0.0.0

; Fuer {commoncf}\VST3 (Program Files) sind Admin-Rechte noetig.
PrivilegesRequired=admin
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
AllowNoIcons=yes

; 64-Bit-Plugin -> 64-Bit-Installation (Inno Setup 6.3+)
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

OutputDir=output
OutputBaseFilename={#AppName}-Setup-{#AppVersion}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Falls ein Icon gewuenscht ist: eine .ico neben diese Datei legen und
; folgende Zeile aktivieren.
;SetupIconFile=echo.ico

[Languages]
Name: "german";  MessagesFile: "compiler:Languages\German.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "vst3";   Description: "Nur VST3-Plugin und Presets"
Name: "full";   Description: "Zusaetzlich die Standalone-Anwendung"
Name: "custom"; Description: "Benutzerdefiniert"; Flags: iscustom

[Components]
Name: "vst3";       Description: "VST3-Plugin (fuer die DAW)";              Types: vst3 full; Flags: fixed
Name: "presets";    Description: "Presets nach {#PresetDir}";              Types: vst3 full; Flags: fixed
Name: "standalone"; Description: "Standalone-Anwendung (eigenstaendig)";  Types: full

[Files]
; --- VST3-Bundle in den Standard-VST3-Ordner -------------------------------
Source: "{#PluginBundle}\*"; DestDir: "{commoncf}\VST3\echo.vst3"; \
    Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3

; --- Standalone-Anwendung --------------------------------------------------
Source: "{#StandaloneExe}"; DestDir: "{app}"; \
    Flags: ignoreversion; Components: standalone

; --- Presets: Ordner anlegen und einspielen --------------------------------
; onlyifdoesntexist  -> vorhandene (evtl. vom Benutzer geaenderte) Presets
;                       werden NICHT ueberschrieben.
; uninsneveruninstall -> Presets bleiben bei einer Deinstallation erhalten.
Source: "{#PresetSource}\*.echo"; DestDir: "{#PresetDir}"; \
    Flags: onlyifdoesntexist uninsneveruninstall; Components: presets

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"; Components: standalone

[UninstallDelete]
; Den Preset-Ordner nur entfernen, wenn er leer ist (Presets bleiben erhalten).
Type: dirifempty; Name: "{#PresetDir}"

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{#AppName} jetzt starten"; \
    Flags: nowait postinstall skipifsilent; Components: standalone
