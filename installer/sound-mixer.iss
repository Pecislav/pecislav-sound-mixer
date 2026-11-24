#ifndef AppVersion
  #define AppVersion "0.2.2"
#endif
#ifndef ProjectRoot
  #define ProjectRoot ".."
#endif

[Setup]
AppId={{0A963BBC-9042-4567-B1CC-F8096D5E6D42}
AppName=Pecislav Sound Mixer
AppVersion={#AppVersion}
AppVerName=Pecislav Sound Mixer
AppPublisher=Pecislav
AppComments=Zvukový mixér pro OBS Studio
#ifndef TestBuild
DefaultDirName={commonappdata}\obs-studio\plugins\pecislav-group-mixer
#else
DefaultDirName={#ProjectRoot}\.deps\installer-check\installed
#endif
DisableDirPage=yes
DisableProgramGroupPage=yes
UsePreviousAppDir=no
WizardStyle=modern
LicenseFile={#ProjectRoot}\LICENSE
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible and not arm64
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=no
RestartApplications=no
SetupMutex=PecislavSoundMixerInstaller
UninstallDisplayName=Pecislav Sound Mixer pro OBS
UninstallDisplayIcon={app}\unins000.exe
VersionInfoVersion={#AppVersion}.0
VersionInfoDescription=Pecislav Sound Mixer Setup

#ifdef TestBuild
PrivilegesRequired=lowest
CreateUninstallRegKey=no
Uninstallable=no
OutputDir={#ProjectRoot}\.deps\installer-check
OutputBaseFilename=SoundMixer-TestSetup
#else
PrivilegesRequired=admin
OutputDir={#ProjectRoot}\dist
OutputBaseFilename=Pecislav-Sound-Mixer-Setup
#endif

[Languages]
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
czech.FinishedLabel=Zvukový mixér je nainstalovaný.%n%nPo spuštění OBS najdeš panel v nabídce Doky → Zvukový mixér. Skupiny, scény a nastavení zvuku zůstávají zachované.
english.FinishedLabel=Sound Mixer is installed.%n%nIn OBS, open Docks → Sound Mixer. The panel follows the language selected in OBS. Existing groups, scenes and audio settings are preserved.

[Files]
Source: "{#ProjectRoot}\build\pecislav-group-mixer.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#ProjectRoot}\data\locale\*.ini"; DestDir: "{app}\data\locale"; Flags: ignoreversion
Source: "{#ProjectRoot}\LICENSE"; DestDir: "{app}\docs"; Flags: ignoreversion
Source: "{#ProjectRoot}\README.cs.md"; DestDir: "{app}\docs"; Flags: ignoreversion

[Run]
Filename: "{code:GetObsExecutable}"; WorkingDir: "{code:GetObsBin}"; Description: "Spustit OBS Studio / Launch OBS Studio"; Flags: postinstall unchecked nowait skipifsilent runasoriginaluser

[Code]
var
  ObsPage: TInputDirWizardPage;

function GetObsRoot: String;
begin
  Result := RemoveBackslash(ObsPage.Values[0]);
end;

function GetObsExecutable(Param: String): String;
begin
  Result := GetObsRoot + '\bin\64bit\obs64.exe';
end;

function GetObsBin(Param: String): String;
begin
  Result := GetObsRoot + '\bin\64bit';
end;

function ObsRunning: Boolean;
var
  Locator, Service, Processes: Variant;
begin
  Result := True;
  try
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Service := Locator.ConnectServer('', 'root\CIMV2');
    Processes := Service.ExecQuery('SELECT ProcessId FROM Win32_Process WHERE Name="obs64.exe"');
    Result := Processes.Count > 0;
  except
    Log('Unable to check running OBS processes; refusing to replace a possibly loaded plugin.');
  end;
end;

function CheckObsVersion: String;
var
  Version: String;
begin
  Result := '';
  if not GetVersionNumbersString(GetObsExecutable(''), Version) then
    Result := 'OBS Studio nebylo nalezeno. Vyber složku s OBS Studio. / OBS Studio was not found. Select its installation folder.'
  else if Pos('33.', Version) <> 1 then
    Result := 'Tento plugin vyžaduje OBS Studio 33.x (testováno 33.0.2). / This plugin requires OBS Studio 33.x (tested with 33.0.2).';
end;

procedure InitializeWizard;
begin
  ObsPage := CreateInputDirPage(wpLicense, 'OBS Studio',
    'Umístění OBS Studio / OBS Studio location',
    'Instalátor přidá Zvukový mixér do OBS. Pokud máš OBS jinde, vyber jeho složku. / Select your OBS installation folder.',
    False, '');
  ObsPage.Add('Složka OBS Studio / OBS Studio folder:');
  ObsPage.Values[0] := ExpandConstant('{commonpf64}\obs-studio');
  if ExpandConstant('{param:OBSROOT|}') <> '' then
    ObsPage.Values[0] := ExpandConstant('{param:OBSROOT|}');
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  Error: String;
begin
  Result := True;
  if (CurPageID = ObsPage.ID) and not WizardSilent then begin
    Error := CheckObsVersion;
    if Error <> '' then begin
      MsgBox(Error, mbError, MB_OK);
      Result := False;
    end;
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := CheckObsVersion;
#ifdef TestBuild
  if ExpandConstant('{param:TESTIGNOREOBS|}') = '1' then Exit;
#endif
  if (Result = '') and ObsRunning then
    Result := 'Před instalací zavři OBS Studio a zkus to znovu. / Close OBS Studio before installing, then retry.';
end;

function InitializeUninstall: Boolean;
begin
  Result := not ObsRunning;
  if not Result then
    MsgBox('Nejdřív zavři OBS Studio. / Close OBS Studio first.', mbError, MB_OK);
end;
