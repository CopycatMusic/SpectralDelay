; CopyCat Spectral Delay - Windows Installer Script
; For use with Inno Setup (https://jrsoftware.org/isdl.php)

#define AppName "CopyCat Spectral Delay"
#define AppVersion "1.0.0"
#define AppPublisher "CopyCat"
#define AppURL "https://github.com/CopycatMusic/SpectralDelay"

[Setup]
; NOTE: The value of AppId uniquely identifies this application. Do not use the same AppId value in installers for other applications.
AppId={{D3F9A1B2-C4D8-4E9F-9123-B68C72A19011}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppURL}
AppSupportURL={#AppURL}
AppUpdatesURL={#AppURL}

; --- CLEANER LAYOUT ---
; We set DefaultDirName to a dedicated App folder so the uninstaller lives there.
; The plugin itself will still be installed to the standard VST3 folder.
DefaultDirName={autopf}\{#AppPublisher}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes

OutputDir=Output
OutputBaseFilename=CopyCat_Spectral_Delay_Win_v{#AppVersion}
Compression=lzma
SolidCompression=yes
WizardStyle=modern
; Requires admin for VST3 and Program Files folder access
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; The plugin bundle (.vst3 folder) is installed to the standard Common Files location
Source: "CopyCat Spectral Delay.vst3\*"; DestDir: "{commoncf64}\VST3\CopyCat Spectral Delay.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Uninstall {#AppName}"; Filename: "{uninstallexe}"

[Messages]
WelcomeLabel2=This will install {#AppName} v{#AppVersion} on your computer.%n%nNote: The plugin will be installed to the standard VST3 location: %nC:\Program Files\Common Files\VST3

[UninstallDelete]
; Ensure the plugin folder is also removed during uninstallation
Type: filesandordirs; Name: "{commoncf64}\VST3\CopyCat Spectral Delay.vst3"
