; ============================================================================
;  FunshiEngineGL - Instalador Windows (Inno Setup 6)
; ============================================================================
;  PARA PUBLICAR UNA BETA / ALPHA / DEMO:
;    1. Edita MiVersion y MiCanal abajo. MiVersion tiene que coincidir con
;       FUNSHI_VERSION de FunshiEngineGL/CMakeLists.txt (la del .exe que se
;       empaqueta): al publicar desde GitHub Actions el numero sale del tag y
;       este archivo se reescribe solo, asi que ahi no hay que tocar nada.
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
; Instalacion POR USUARIO, sin UAC: el motor no necesita escribir junto al
; ejecutable para funcionar. Toda escritura cuelga de la raiz que resuelve el
; propio motor (ProjectPaths::directorioBase): {app}\MotorGrafico si ahi se
; puede escribir y, si no, la carpeta del usuario. Con lowest, Inno mapea
; {autopf} a la version de usuario (%LOCALAPPDATA%\Programs), de modo que la
; carpeta de datos tambien queda en el perfil y nunca en Program Files.
; Verificado con el ejecutable real: con su carpeta de solo lectura no escribe
; nada ahi y todo cae en la carpeta de datos del usuario.
; Lo unico que puede pedir elevacion es el MSI del JDK de la seccion [Code],
; porque deja JAVA_HOME en el entorno de la maquina.
PrivilegesRequired=lowest
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
Source: "include\*"; DestDir: "{app}\include"; Flags: ignoreversion recursesubdirs
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
const
  { El salto de linea va en una constante y no suelto como #13#10 porque ISPP
    (el preprocesador de Inno) interpreta cualquier linea que arranque con # como
    una directiva y aborta con "Unknown preprocessor directive". }
  NL = #13#10;

var
  JdkAdvertencia: string;

{ Un JDK utilizable en <Raiz>: la biblioteca de la JVM Y el compilador. Comprobar
  solo jvm.dll daba un falso positivo con un JRE: el instalador decia "JDK
  detectado" y el motor no podia compilar ningun .java. El predicado es el mismo
  que aplica el motor (libjvmEnRaiz + javacEnRaiz en BackendJava.cpp), con los
  nombres de archivo de Windows. }
function JdkCompletoEn(const Raiz: string): Boolean;
begin
  Result := FileExists(Raiz + '\bin\server\jvm.dll') and
            FileExists(Raiz + '\bin\javac.exe');
end;

// JDK completo en <base>\<algo>\. Se recorren las carpetas donde los JDK se
// instalan de verdad en vez de consultar el registro, que en Pascal Script exige
// enumerar subclaves a mano.
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
        if (Buscador.Attributes and FILE_ATTRIBUTE_DIRECTORY) <> 0 then
        begin
          Sub := Buscador.Name;
          if JdkCompletoEn(Carpeta + '\' + Sub) then
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
    Result := Valor;
end;

function DetectarJdk(): Boolean;
var
  Home: string;
begin
  { 1. JRE embebido junto al ejecutable: tiene que traer javac, igual que en
     cualquier otra ruta (si solo trae la JVM, el motor no puede compilar) }
  if JdkCompletoEn(ExpandConstant('{app}\jre')) then
  begin
    Result := True;
    Exit;
  end;
  { 2. JAVA_HOME, primero el del proceso y luego el de la maquina }
  if JdkCompletoEn(ExpandConstant('{env:JAVA_HOME}')) then
  begin
    Result := True;
    Exit;
  end;
  Home := JavaHomeMaquina();
  if (Home <> '') and JdkCompletoEn(Home) then
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
  Msi: string;
  Bytes: Int64;
  Codigo: Integer;
begin
  Result := '';
  NeedsRestart := False;
  JdkAdvertencia := '';

  if DetectarJdk() then
    Exit;

  if WizardSilent then
    Exit;  { en /VERYSILENT no se pregunta nada }

  if MsgBox('No se encontro un JDK (Java Development Kit) en este equipo.' + NL + NL +
            'Los scripts Java del motor necesitan un JDK porque compila los .java ' +
            'del proyecto con javac antes de cargarlos en la JVM. Sin el, el editor ' +
            'abre igual pero los scripts Java no van a funcionar.' + NL + NL +
            'Deseas descargar e instalar Temurin JDK 17 ahora? Son unos 190 MB y ' +
            'requiere conexion a internet.',
            mbConfirmation, MB_YESNO) = IDNO then
  begin
    JdkAdvertencia := 'No se instalo un JDK, asi que los scripts Java no van a funcionar.';
    Exit;
  end;

  { DownloadTemporaryFile deja el archivo en la carpeta temporal con el nombre
    pedido y muestra el progreso con su propio dialogo cuando
    OnDownloadProgress es nil. Si algo falla (sin red, URL caida) levanta
    excepcion, de ahi el except. }
  Msi := ExpandConstant('{tmp}\temurin-jdk17.msi');
  try
    Bytes := DownloadTemporaryFile('{#JdkUrl}', 'temurin-jdk17.msi', '', nil);
  except
    Bytes := 0;
  end;
  if (Bytes <= 0) or not FileExists(Msi) then
  begin
    JdkAdvertencia := 'No se pudo descargar el JDK, asi que los scripts Java no van ' +
                     'a funcionar.';
    Exit;
  end;

  { FeatureMain: el JDK. FeatureEnvironment: deja JAVA_HOME y el PATH.
    FeatureJavaHome/FeatureJarFileRunWith: completan la instalacion estandar.
    /qn es la instalacion silenciosa del propio MSI. }
  if not Exec(Msi,
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

  { El MSI se instala por maquina, asi que ESTE es el paso que puede pedir
    elevacion aunque el resto de la instalacion no la pida.
    El MSI deja JAVA_HOME en el entorno de la maquina, pero este proceso ya
    estaba corriendo y no lo ve. No hace falta pasarselo a mano: el motor
    descubre el JDK por su cuenta en el registro y en Program Files, asi que
    lo encuentra igual aunque JAVA_HOME no sea visible todavia. }
  if not DetectarJdk() then
    JdkAdvertencia := 'El JDK se instalo, pero no se pudo localizar. Si los scripts Java ' +
                     'no funcionan, reinicia Windows o configura JAVA_HOME a mano.';
end;

{ Si al terminar no hay JDK, se avisa una vez con un mensaje claro. }
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and (JdkAdvertencia <> '') then
    MsgBox(JdkAdvertencia + NL + NL +
           'Se puede instalar despues desde https://adoptium.net',
           mbInformation, MB_OK);
end;
