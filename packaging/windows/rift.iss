; Rift installer (Inno Setup 6). Built by CI:
;   iscc /DAppVersion=1.2.3 /DSourceDir=<deployed folder> /DOutputDir=<dist> packaging\windows\rift.iss
; Per-user install by default (no admin prompt); the user can choose "all users" in the wizard.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\build\deploy"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

[Setup]
AppId={{6B0E7C5A-3D1F-4E8B-9A2C-71F0D5B3A9E4}
AppName=Rift
AppVersion={#AppVersion}
AppVerName=Rift {#AppVersion}
AppPublisher=Rift
AppPublisherURL=https://rift.solarquack.in
AppSupportURL=https://github.com/jeelnadaa/rift/issues
AppUpdatesURL=https://rift.solarquack.in/download
DefaultDirName={autopf}\Rift
DefaultGroupName=Rift
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=Rift-{#AppVersion}-windows-x64-setup
SetupIconFile=..\..\ui\assets\rift.ico
UninstallDisplayIcon={app}\Rift.exe
UninstallDisplayName=Rift
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
VersionInfoVersion={#AppVersion}
MinVersion=10.0.17763

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "magnet"; Description: "Open magnet links and .torrent files with Rift"; GroupDescription: "Associations:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Rift"; Filename: "{app}\Rift.exe"
Name: "{autodesktop}\Rift"; Filename: "{app}\Rift.exe"; Tasks: desktopicon

[Registry]
; magnet: protocol and .torrent files (per user or machine, following the install mode)
Root: HKA; Subkey: "Software\Classes\magnet"; ValueType: string; ValueName: ""; ValueData: "URL:Magnet link"; Flags: uninsdeletekey; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\magnet"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\magnet\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\Rift.exe"",0"; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\magnet\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Rift.exe"" ""%1"""; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\.torrent"; ValueType: string; ValueName: ""; ValueData: "Rift.Torrent"; Flags: uninsdeletevalue; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\Rift.Torrent"; ValueType: string; ValueName: ""; ValueData: "Torrent file"; Flags: uninsdeletekey; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\Rift.Torrent\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\Rift.exe"",0"; Tasks: magnet
Root: HKA; Subkey: "Software\Classes\Rift.Torrent\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\Rift.exe"" ""%1"""; Tasks: magnet
; Native messaging host keys are written by Rift on first start; remove them on uninstall.
Root: HKCU; Subkey: "Software\Google\Chrome\NativeMessagingHosts\app.rift.native"; Flags: uninsdeletekey dontcreatekey
Root: HKCU; Subkey: "Software\Microsoft\Edge\NativeMessagingHosts\app.rift.native"; Flags: uninsdeletekey dontcreatekey
Root: HKCU; Subkey: "Software\BraveSoftware\Brave-Browser\NativeMessagingHosts\app.rift.native"; Flags: uninsdeletekey dontcreatekey
Root: HKCU; Subkey: "Software\Chromium\NativeMessagingHosts\app.rift.native"; Flags: uninsdeletekey dontcreatekey
Root: HKCU; Subkey: "Software\Vivaldi\NativeMessagingHosts\app.rift.native"; Flags: uninsdeletekey dontcreatekey
Root: HKCU; Subkey: "Software\Mozilla\NativeMessagingHosts\app.rift.native"; Flags: uninsdeletekey dontcreatekey
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueName: "Rift"; Flags: uninsdeletevalue dontcreatekey

[Run]
Filename: "{app}\Rift.exe"; Description: "{cm:LaunchProgram,Rift}"; Flags: nowait postinstall skipifsilent

[UninstallRun]
Filename: "{cmd}"; Parameters: "/C taskkill /IM Rift.exe /F"; Flags: runhidden; RunOnceId: "KillRift"
