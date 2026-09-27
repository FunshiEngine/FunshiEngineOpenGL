; ============================================================================
;  FunshiEngineGL - Instalador Windows (Inno Setup 6)
; ============================================================================
;  PARA PUBLICAR UNA BETA / ALPHA / DEMO:
;    1. Edita MiVersion y MiCanal abajo.
;    2. Ejecuta HacerInstalador.bat (monta packaging/dist/ y compila este
;       script con ISCC). Alternativa manual: ISCC.exe FunshiEngineGL_setup.iss
;    3. El instalador queda en packaging/instalador/.
;
;  Cada canal genera un archivo distinto (evita confusiones entre builds):
;    Demo  -> FunshiEngineGL-0.5.0-demo-setup.exe
;    Alpha -> FunshiEngineGL-0.5.0-alpha-setup.exe
;    Beta  -> FunshiEngineGL-0.5.0-beta-setup.exe
; ============================================================================

#define MiNombre "FunshiEngineGL"
#define MiVersion "0.5.0"
#define MiCanal "alpha"          ; demo | alpha | beta | rc
#define MiExe "FunshiEngineGL.exe"
; MiEdicion se construye con + (expresion evaluada por ISPP): un literal de
; cadena no expande las {#...} internas y quedarian como texto crudo.
#define MiEdicion MiNombre + " " + MiVersion + " (" + MiCanal + ")"
#define MiId "{{62F2D6B7-8C4E-4A1B-B3D5-9E7A1F0C2B8A}}"

[Setup]
AppId={#MiId}
AppName={#MiEdicion}
AppVersion={#MiVersion}-{#MiCanal}
AppVerName={#MiEdicion}
AppPublisher=FunshiEngineGL
AppComments=Camara y editor de escenas 3D con OpenGL e ImGui
DefaultDirName={autopf}\FunshiEngineGL
DefaultGroupName=FunshiEngineGL
UninstallDisplayIcon={app}\{#MiExe}
Compression=lzma2
SolidCompression=yes
SourceDir=dist
OutputDir=instalador
OutputBaseFilename=FunshiEngineGL-{#MiVersion}-{#MiCanal}-setup
; x64 solamente: el proyecto usa Assimp/Bullet/GLFW de 64 bits (vcpkg x64-windows).
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; El editor guarda escena y configuracion en {app}\MotorGrafico (junto al exe).
; Crear ese directorio y subcarpetas requiere permisos de administrador.
PrivilegesRequired=admin
SetupLogging=yes

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Crear acceso directo en el escritorio"; GroupDescription: "Accesos directos:"

[Files]
Source: "{#MiExe}"; DestDir: "{app}"; Flags: ignoreversion
Source: "*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "Imagenes\*"; DestDir: "{app}\Imagenes"; Flags: ignoreversion recursesubdirs
Source: "..\..\..\LICENSE"; DestDir: "{app}\licencia"; Flags: ignoreversion
Source: "..\..\..\NOTICE"; DestDir: "{app}\licencia"; Flags: ignoreversion
Source: "..\..\..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}\licencia"; Flags: ignoreversion

; ============================================================================
; MotorGrafico solo contiene lo que el usuario crea: sus proyectos
; (<nombre>\Memory\Binarios\Scene y <nombre>\src<nombre>) y la configuracion
; (Configuracion.json + imgui.ini). No se pre-crean Modelos/Texturas/Imagenes:
; eso lo decide el editor al crear cada proyecto (EditorConfig.cpp).
; Se crea la raiz vacia para que el primer arranque no falle al escribir
; Configuracion.json (SceneSerializer NO crea directorios por su cuenta).
; ============================================================================
[Dirs]
Name: "{app}\MotorGrafico"

[Icons]
Name: "{group}\{#MiNombre} {#MiVersion} ({#MiCanal})"; Filename: "{app}\{#MiExe}"; WorkingDir: "{app}"
Name: "{group}\Carpeta del proyecto (MotorGrafico)"; Filename: "{app}\MotorGrafico"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MiNombre} {#MiVersion} ({#MiCanal})"; Filename: "{app}\{#MiExe}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MiExe}"; Description: "Ejecutar {#MiNombre} ahora"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent

; ============================================================================
;  Requisito de JDK
;
;  El backend Java del motor compila el .java del usuario con javac y lo carga
;  en una JVM, asi que necesita un JDK (no alcanza un JRE). El instalador NO
;  empaqueta ningun runtime: si falta, se ofrece descargarlo e instalarlo.
;  El motor (rutaLibjvm/javacEnRaiz en BackendJava.cpp) busca el JDK por su
;  cuenta en JAVA_HOME, en el registro y en las carpetas usuales, asi que con
;  que JAVA_HOME quede seteado alcanza.
; ============================================================================
#define JdkUrl "https://api.adoptium.net/v3/installer/latest/17/ga/windows/x64/jdk/hotspot/normal/eclipse"

[Code]
var
  JdkAdvertencia: string;

// Un JDK en <base>\<algo>\bin\server\jvm.dll. Se recorren las carpetas donde
// los JDK se instalan de verdad en vez de consultar el registro, que en
// Pascal Script exige enumerar subclaves a mano.
function HayJdkEn(const Carpeta: string): Boolean;
var
  Buscador: TFindRec;
  Sub: string;
begin
  Result := False;
  if (Carpeta = '') or not DirExists(Carpeta) then Exit;
  if FindFirst(Carpeta + '\*', Buscador) then
  begin
    try
      repeat
        if (Buscador.Attr and FILE_ATTRIBUTE_DIRECTORY) <> 0 then
        begin
          Sub := Buscador.Name;
          if FileExists(Carpeta + '\' + Sub + '\bin\server\jvm.dll') then
          begin
            Result := True;
            Exit;
          end;
        end;
      until not FindNext(Buscador);
    finally
      FindClose(Buscador);
    end;
  end;
end;

// JAVA_HOME de la maquina. GetEnv solo ve el entorno del proceso de setup, que
// no incluye lo que se acaba de instalar, asi que para el JDK recien puesto se
// lee del registro.
function JavaHomeMaquina(): string;
var
  Valor: string;
begin
  Result := '';
  if RegQueryStringValue(HKEY_LOCAL_MACHINE,
      'SYSTEM\CurrentControlSet\Control\Session Manager\Environment',
      'JAVA_HOME', Valor) then
  begin
    Result := Valor;
    while (Result <> '') and (Result[Length(Result)] = '\') do
      Result := Delete(Result, Length(Result), 1);
  end;
end;

function DetectarJdk(): Boolean;
var
  Home: string;
begin
  { 1. JRE embebido junto al ejecutable }
  if FileExists(ExpandConstant('{app}\jre\bin\server\jvm.dll')) then
  begin
    Result := True;
    Exit;
  end;
  { 2. JAVA_HOME, primero el del proceso y luego el de la maquina }
  if FileExists(ExpandConstant('{env:JAVA_HOME}\bin\server\jvm.dll')) then
  begin
    Result := True;
    Exit;
  end;
  Home := JavaHomeMaquina();
  if (Home <> '') and FileExists(Home + '\bin\server\jvm.dll') then
  begin
    Result := True;
    Exit;
  end;
  { 3. Instalaciones tipicas }
  Result := HayJdkEn(ExpandConstant('{commonpf}\Java')) or
            HayJdkEn(ExpandConstant('{commonpf}\Eclipse Adoptium')) or
            HayJdkEn(ExpandConstant('{commonpf}\Microsoft')) or
            HayJdkEn(ExpandConstant('{commonpf}\Amazon Corretto')) or
            HayJdkEn(ExpandConstant('{commonpf}\Zulu')) or
            HayJdkEn(ExpandConstant('{commonpf32}\Java')) or
            HayJdkEn(ExpandConstant('{localappdata}\Programs\Eclipse Adoptium'));
end;

{ Preparacion: si no hay JDK, se ofrece traer Adoptium Temurin 17. El MSI se
  instala con FeatureEnvironment para que ademas deje JAVA_HOME en la maquina.
  Nunca se aborta la instalacion del motor por esto: si el usuario no quiere o
  no hay red, se sigue y se avisa al final. }
function PrepareToInstall(var NeedsRestart: Boolean): string;
var
  Descarga: TDownloadTemporaryFile;
  Codigo: Integer;
  Home: string;
begin
  Result := '';
  NeedsRestart := False;
  JdkAdvertencia := '';

  if DetectarJdk() then
    Exit;

  if WizardSilent then
    Exit;  { en /VERYSILENT no se pregunta nada }

  if MsgBox('No se encontro un JDK (Java Development Kit) en este equipo.' + #13#10 +
            #13#10 +
            'Los scripts Java del motor necesitan un JDK porque compila los .java ' +
            'del proyecto con javac antes de cargarlos en la JVM. Sin el, el editor ' +
            'abre igual pero los scripts Java no van a funcionar.' + #13#10 + #13#10 +
            'Deseas descargar e instalar Temurin JDK 17 ahora? Son unos 190 MB y ' +
            'requiere conexion a internet.',
            mbConfirmation, MB_YESNO) = IDNO then
  begin
    JdkAdvertencia := 'No se instalo un JDK, asi que los scripts Java no van a funcionar.';
    Exit;
  end;

  Descarga := TDownloadTemporaryFile.Create(
    '{#JdkUrl}', 'Descargando Temurin JDK 17...', 'temurin-jdk17.msi');
  try
    if not Descarga.ShowWaitDialog then
    begin
      JdkAdvertencia := 'Se cancelo la descarga del JDK, asi que los scripts Java no van a funcionar.';
      Exit;
    end;
  finally
    Descarga.Free;
  end;

  { FeatureMain: el JDK. FeatureEnvironment: deja JAVA_HOME y el PATH.
    FeatureJavaHome/FeatureJarFileRunWith: completan la instalacion estandar.
    /qn es la instalacion silenciosa del propio MSI. }
  if not Exec(Descarga.Filename,
    '/qn /norestart ADDLOCAL=FeatureMain,FeatureEnvironment,FeatureJavaHome,FeatureJarFileRunWith',
    '', SW_HIDE, True, Codigo) then
  begin
    JdkAdvertencia := 'No se pudo lanzar el instalador del JDK. Los scripts Java no ' +
                     'van a funcionar hasta instalar un JDK.';
    Exit;
  end;

  if (Codigo <> 0) then
  begin
    JdkAdvertencia := 'La instalacion del JDK termino con codigo ' + IntToStr(Codigo) +
      '. Los scripts Java no van a funcionar hasta instalar un JDK.';
    Exit;
  end;

  { El MSI recien instalado todavia no figura en el entorno de este proceso, y
    el motor se lanza desde aca ([Run]), asi que se le pasa JAVA_HOME a mano. }
  Home := JavaHomeMaquina();
  if Home <> '' then
    SetEnv('JAVA_HOME', Home);

  if not DetectarJdk() then
    JdkAdvertencia := 'El JDK se instalo, pero no se pudo localizar. Si los scripts Java ' +
                     'no funcionan, reinicia Windows o configura JAVA_HOME a mano.';
end;

{ Si al terminar no hay JDK, se avisa una vez con un mensaje claro. }
function CurStepChanged(CurStep: TSetupStep): string;
begin
  Result := '';
  if (CurStep = ssPostInstall) and (JdkAdvertencia <> '') then
    MsgBox(JdkAdvertencia + #13#10 + #13#10 +
           'Se puede instalar despues desde https://adoptium.net',
           mbInformation, MB_OK);
end;