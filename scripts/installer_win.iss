; CopyCat Spectral Delay - Windows Installer Script
; For use with Inno Setup (https://jrsoftware.org/isdl.php)
;
; Usage (local):
;   Place this file alongside the plugin folder and run with Inno Setup
;   Expected structure:
;     release/vX.X.X/Windows/
;       ├── CopyCat Spectral Delay.vst3/
;       └── installer_win.iss (copy this file here)
;
; Usage (CI):
;   ISCC.exe /DAppVersion="X.X.X" /DSourcePath="path\to\vst3\folder" installer_win.iss

#ifndef AppVersion
  #define AppVersion "1.1.0"
#endif

#ifndef SourcePath
  #define SourcePath "."
#endif

#define AppName "CopyCat Spectral Delay"
#define AppPublisher "CopyCat"
#define AppURL "https://github.com/CopycatMusic/SpectralDelay"

[Setup]
AppId={{D3F9A1B2-C4D8-4E9F-9123-B68C72A19011}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}

DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes

OutputDir=Output
OutputBaseFilename=CopyCat_Spectral_Delay_Win_v{#AppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; The plugin bundle (.vst3 folder) - relative path from SourcePath
Source: "{#SourcePath}\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Messages]
WelcomeLabel2=This will install {#AppName} v{#AppVersion} on your computer.%n%nThe plugin will be installed to:%nC:\Program Files\Common Files\VST3

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"
