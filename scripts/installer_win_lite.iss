; CopyCat Spectral Delay Lite - Windows Installer Script (Lite Version)
; For use with Inno Setup (https://jrsoftware.org/isdl.php)
;
; Usage: Place this file alongside the plugin folder and run with Inno Setup
; Expected structure:
;   release/vX.X.X/Windows/Lite/
;     ├── CopyCat Spectral Delay Lite.vst3/
;     └── installer_win_lite.iss (copy this file here)

#define AppName "CopyCat Spectral Delay Lite"
#define AppVersion "1.1.0"
#define AppPublisher "CopyCat"
#define AppURL "https://github.com/CopycatMusic/SpectralDelay"

[Setup]
; Different AppId for Lite version - allows both to be installed
AppId={{E4F0B2C3-D5E9-4F0A-A234-C79D83B2A122}
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
OutputBaseFilename=CopyCat_Spectral_Delay_Lite_Win_v{#AppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; The plugin bundle (.vst3 folder) - relative path from script location
Source: "CopyCat Spectral Delay Lite.vst3\*"; DestDir: "{commoncf64}\VST3\CopyCat Spectral Delay Lite.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Messages]
WelcomeLabel2=This will install {#AppName} v{#AppVersion} on your computer.%n%nThe plugin will be installed to:%nC:\Program Files\Common Files\VST3

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\CopyCat Spectral Delay Lite.vst3"
