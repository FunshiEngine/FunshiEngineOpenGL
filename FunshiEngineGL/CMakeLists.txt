cmake_minimum_required(VERSION 3.15)
project(FunshiEngineGL VERSION 0.5.4 LANGUAGES CXX)

# Versión del producto: fuente única para el recurso VERSIONINFO del .exe de
# Windows (packaging/icon/app.rc.in); subir al cortar releases.
set(FUNSHI_VERSION "0.5.4")
# Sustituciones de configure_file para el .rc: el icono se referencia relativo
# al .rc (se copia al build), la version se lleva a comas (1,2,3,0).
set(FUNSHI_APP_ICON "app.ico")
string(REPLACE "." "," FUNSHI_VERSION_COMMA "${FUNSHI_VERSION}")
set(FUNSHI_VERSION_COMMA "${FUNSHI_VERSION_COMMA},0")

# Configuración básica
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Configuración de rutas
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)

# Búsqueda de archivos fuente
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS ${SRC_DIR}/*.cpp ${SRC_DIR}/*.c)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS ${SRC_DIR}/*.h ${SRC_DIR}/*.hpp)

# El motor se compila como libreria estatica (funshi_engine) y el ejecutable
# solo aporta el punto de entrada (main.cpp). Asi el engine es reutilizable
# (tests, herramientas, otros front-ends) sin recompilar todo. CI puede compilar
# SOLO las pruebas headless con BUILD_ENGINE=OFF.
option(BUILD_ENGINE "Build engine library and executable" ON)
# Soporte de scripts Java via JNI: requiere JDK en tiempo de build para jni.h.
# El JVM en runtime se carga dinamicamente solo si hay una escena con .java.
# Por defecto ON: si el build encuentra el JDK se habilita solo, si no, el
# backend Java se omite sin romper la compilacion de nada.
option(FUNSHI_JAVA "Soporte de scripts Java via JNI (requiere JDK en build)" ON)

if(BUILD_ENGINE)
    # main.cpp es el punto de entrada del ejecutable, no parte de la libreria.
    list(REMOVE_ITEM SOURCES ${SRC_DIR}/main.cpp)

    # El backend Java solo se compila con FUNSHI_JAVA=ON: necesita jni.h del
    # JDK en tiempo de build. Igual en runtime el JVM es dinamico/opcional.
    list(REMOVE_ITEM SOURCES ${SRC_DIR}/Behaviour/Backends/BackendJava.cpp)

    add_library(funshi_engine STATIC ${SOURCES} ${HEADERS})

    # Ejecutable principal (solo el entry point).
    add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cpp)
    target_link_libraries(${PROJECT_NAME} PRIVATE funshi_engine)
    # En Windows el binario es GUI (sin consola): WinMain arma argc/argv desde
    # GetCommandLineW y delega en el arranque comun, y la salida va al log, asi
    # no se abre ninguna terminal. En Linux no tiene efecto.
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} PRIVATE shell32)
        # Recurso Windows del ejecutable: icono (app.ico) + bloque VERSIONINFO
        # con CompanyName/ProductVersion. El .rc se genera por configure_file
        # al directorio de build (junto a una copia del .ico) y el compilador de
        # recursos de MSVC/MinGW lo enlaza al .exe.
        configure_file(
            ${CMAKE_SOURCE_DIR}/packaging/icon/app.rc.in
            ${CMAKE_BINARY_DIR}/FunshiEngineGL.rc @ONLY)
        file(COPY ${CMAKE_SOURCE_DIR}/packaging/icon/app.ico
             DESTINATION ${CMAKE_BINARY_DIR})
        target_sources(${PROJECT_NAME} PRIVATE
            ${CMAKE_BINARY_DIR}/FunshiEngineGL.rc)
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_include_directories(funshi_engine PUBLIC ${SRC_DIR})
    target_compile_definitions(funshi_engine PUBLIC GLM_ENABLE_EXPERIMENTAL)

    # Soporte opcional de scripts Java (JNI). Se detecta el JDK con FindJNI y
    # se embeben las rutas de javac/libjvm para el runtime (se pueden
    # sobreescribir con las variables de entorno JAVA_HOME/JAVAC/FUNSHI_LIBJVM).
    # Sin JDK en el sistema el backend simplemente no se compila, y la GUI lo
    # reporta como "sin soporte".
    find_package(JNI QUIET)
    if(FUNSHI_JAVA AND JNI_FOUND)
        find_package(Java QUIET COMPONENTS Development)
        target_sources(funshi_engine PRIVATE
            ${SRC_DIR}/Behaviour/Backends/BackendJava.cpp)
        target_include_directories(funshi_engine PRIVATE ${JNI_INCLUDE_DIRS})
        target_compile_definitions(funshi_engine PUBLIC FUNSHI_JAVA)
        if(Java_JAVAC_EXECUTABLE)
            target_compile_definitions(funshi_engine
                PRIVATE FUNSHI_JAVAC_DEFAULT="${Java_JAVAC_EXECUTABLE}")
        endif()
        if(JAVA_JVM_LIBRARY)
            target_compile_definitions(funshi_engine
                PRIVATE FUNSHI_LIBJVM_DEFAULT="${JAVA_JVM_LIBRARY}")
        endif()
        if(UNIX)
            target_link_libraries(funshi_engine PRIVATE ${CMAKE_DL_LIBS})
        endif()
        message(STATUS
            "Scripts Java HABILITADOS (javac: ${Java_JAVAC_EXECUTABLE}).")
    elseif(FUNSHI_JAVA)
        message(STATUS
            "FUNSHI_JAVA=ON pero no se encontro el JDK (jni.h): el backend "
            "Java queda desactivado.")
    endif()

    # Ruta al fuente del motor (para que los scripts usen las cabeceras al
    # compilarse en caliente) y compilador C++ a invocar por el BackendCpp.
    # En Windows los backslashes del path se escapan para el literal C++; el
    # BackendCpp entrecomilla la ruta al armar el argumento -I al usarla.
    set(FUNSHI_SRC_ESCAPADO "${SRC_DIR}")
    if(WIN32)
        string(REPLACE "\\" "\\\\" FUNSHI_SRC_ESCAPADO
               "${FUNSHI_SRC_ESCAPADO}")
    endif()
    target_compile_definitions(funshi_engine
        PUBLIC FUNSHI_SRC_DIR="${FUNSHI_SRC_ESCAPADO}"
               FUNSHI_CXX_COMPILER="${CMAKE_CXX_COMPILER}")

# 1. Configuración de GLFW
find_package(glfw3 REQUIRED)
target_link_libraries(funshi_engine PRIVATE glfw)

# 2. Configuración de OpenGL y GLU (VERSIÓN CORREGIDA)
find_package(OpenGL REQUIRED)

# Primero enlazamos OpenGL básico
if(TARGET OpenGL::OpenGL)
    target_link_libraries(funshi_engine PRIVATE OpenGL::OpenGL)
else()
    target_link_libraries(funshi_engine PRIVATE ${OPENGL_LIBRARIES})
endif()

# Bullet Physics
find_package(Bullet REQUIRED)

# Si Bullet define targets modernos
if(TARGET Bullet::BulletDynamics)
    target_link_libraries(funshi_engine PRIVATE
        Bullet::BulletDynamics
        Bullet::BulletCollision
        Bullet::LinearMath
    )
else()
    # Método clásico: uso de variables
    target_include_directories(funshi_engine PUBLIC ${BULLET_INCLUDE_DIRS})
    target_link_libraries(funshi_engine PRIVATE
        ${BULLET_LIBRARIES}
    )
endif()

# --- Sanitizers --------------------------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer for Debug builds" ON)

# Asegurate de generar en Debug para mejores trazas:
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# Los flags viven en un target INTERFACE para no duplicarlos: se aplican a la
# libreria y, por propagacion de usage requirements, tambien al ejecutable que
# la enlaza (los link options deben llegar al binario final, no al .a).
add_library(funshi_sanitizers INTERFACE)
if(ENABLE_ASAN AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
  message(STATUS "Building with AddressSanitizer (ASan)")
  # Compilación: ASan + mejores stack traces + símbolos + optimización ligera
  target_compile_options(funshi_sanitizers INTERFACE
    -fsanitize=address
    -fno-omit-frame-pointer
    -O1
    -g
  )
  # Enlace: ASan (importante agregar también al linkeo)
  target_link_options(funshi_sanitizers INTERFACE
    -fsanitize=address
  )

  # Opcional pero útil: UndefinedBehaviorSanitizer (UBSan)
  # (si molesta en 3rd-party libs, desactivalo)
  target_compile_options(funshi_sanitizers INTERFACE -fsanitize=undefined)
  target_link_options(funshi_sanitizers INTERFACE -fsanitize=undefined)
endif()
target_link_libraries(funshi_engine PUBLIC funshi_sanitizers)
# ---------------------------------------------------------------------------


# Configuración para Windows
if(WIN32)
    # dbghelp: CapturaBackTrace/SymFromAddr/SymInitialize (RuntimeException).
    # En MinGW los #pragma comment(lib) de MSVC se ignoran, hay que enlazarla.
    target_link_libraries(funshi_engine PRIVATE opengl32 dbghelp)
    # Evitar conflicto std::byte (C++17) vs typedef byte en headers Windows (rpcndr.h, etc.)
    target_compile_definitions(funshi_engine PRIVATE _HAS_STD_BYTE=0)
endif()

# Dependencias específicas de Linux
if(UNIX AND NOT APPLE)
    target_link_libraries(funshi_engine PRIVATE
        dl
        X11
        pthread
        Xrandr
        Xi
    )

    # Ncurses
    find_package(Curses REQUIRED)
    target_include_directories(funshi_engine PUBLIC ${CURSES_INCLUDE_DIRS})
    target_link_libraries(funshi_engine PRIVATE ${CURSES_LIBRARIES})
endif()

# ImGui
file(GLOB IMGUI_SRC ${SRC_DIR}/ImGui/*.cpp ${SRC_DIR}/ImGui/backends/*.cpp)
target_sources(funshi_engine PRIVATE ${IMGUI_SRC})
target_include_directories(funshi_engine PUBLIC ${SRC_DIR}/ImGui ${SRC_DIR}/ImGui/backends)

# ImGuizmo (fuera de src)
file(GLOB IMGUZIMO_SRC ${CMAKE_SOURCE_DIR}/ImGuizmo/*.cpp)
target_sources(funshi_engine PRIVATE ${IMGUZIMO_SRC})
target_include_directories(funshi_engine PUBLIC ${CMAKE_SOURCE_DIR}/ImGuizmo)

# nlohmann/json (vendoriado en External/nlohmann) para la configuracion del
# editor (EditorConfig). Mismamente el nombre .hpp cuelga de External/.
target_include_directories(funshi_engine PUBLIC ${CMAKE_SOURCE_DIR}/External)

# Assimp
option(USE_ASSIMP "Enable Assimp model loading" ON)
if(USE_ASSIMP)
    find_package(assimp REQUIRED)
    target_include_directories(funshi_engine PUBLIC ${ASSIMP_INCLUDE_DIRS})
    target_link_libraries(funshi_engine PRIVATE assimp::assimp)
endif()

# GLM: se usa el directorio embebido si existe en el repo; si no (p. ej. en CI
# sin External/), se busca la copia del sistema (libglm-dev en Ubuntu).
set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/External/glm")
if(EXISTS "${GLM_INCLUDE_DIR}/glm/glm.hpp")
    target_include_directories(funshi_engine PUBLIC ${GLM_INCLUDE_DIR})
else()
    find_package(glm REQUIRED)
    target_link_libraries(funshi_engine PRIVATE glm::glm)
endif()

# Comandos (sistema undo/redo): se enlaza con funshi_engine que provee todas las
# dependencias del motor (ImGui, componentes, EditorController, SceneRegistry).
# Solo se compila el archivo de test; las fuentes del motor vienen del engine.
add_executable(comandos-tests
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/ComandosTests.cpp
)
target_include_directories(comandos-tests PRIVATE
    ${SRC_DIR}
    ${CMAKE_SOURCE_DIR}/External
    ${CMAKE_SOURCE_DIR}/External/imgui
)
target_link_libraries(comandos-tests PRIVATE funshi_engine)
set_target_properties(comandos-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME comandos-tests COMMAND comandos-tests)

if(WIN32)
    target_link_libraries(comandos-tests PRIVATE dbghelp)
    target_compile_definitions(comandos-tests PRIVATE _HAS_STD_BYTE=0)
endif()

# Nucleo del manifiesto de assets de la escena (SceneAssets.json): headless,
# sin GameObjects (como MenuModel). Cubre JSON round-trip, tolerancia a
# archivos corruptos/inexistentes, relativizacion/absolutizacion contra la
# raiz de assets y la regla de precedencia del manifiesto sobre el .db.
add_executable(manifiesto-assets-tests
    ${SRC_DIR}/Configuracion/ProjectPaths.cpp
    ${SRC_DIR}/Configuracion/ProjectManager.cpp
    ${SRC_DIR}/Configuracion/ConfigPersistence.cpp
    ${SRC_DIR}/Scenes/ManifiestoAssetsCore.cpp
    ${SRC_DIR}/Configuracion/EditorConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/ManifiestoAssetsTests.cpp
)
target_include_directories(manifiesto-assets-tests PRIVATE
    ${SRC_DIR}
    ${CMAKE_SOURCE_DIR}/External
)
set_target_properties(manifiesto-assets-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME manifiesto-assets-tests COMMAND manifiesto-assets-tests)

# Orquestador de estados de GUI (la "funcion de marco" de F5/F6/F7 y las teclas)
# tambien es pura logica de estados y se prueba headless: cubre las reglas por
# estado de Play/Pausa/Stop, Escape -> menu e "Iniciar Estudio" -> editor.
add_executable(orquestador-estado-tests
    ${SRC_DIR}/States/ApplicationStateMachine.cpp
    ${SRC_DIR}/States/OrquestadorEstadoGUI.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/OrquestadorEstadoTests.cpp
)
set_target_properties(orquestador-estado-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME orquestador-estado-tests COMMAND orquestador-estado-tests)

# TemaEditor (aplicacion del perfil de apariencia al estilo de ImGui) se prueba
# headless como los anteriores: el estilo solo necesita el contexto de ImGui,
# sin backends ni pila grafica. Cubre la regresion "el color de acento no llega
# a toda la interfaz": verifica que con un acento no azul NINGUN rol de la
# paleta conserve el azul de fabrica de Dear ImGui (FrameBg, Tab, Border,
# TableHeaderBg, TextLink, DragDropTarget...), que con el acento por defecto no
# cambie el aspecto historico, y que el modo B/N siga dejando todo monocromo.
add_executable(tema-tests
    ${SRC_DIR}/ImGui/imgui.cpp
    ${SRC_DIR}/ImGui/imgui_draw.cpp
    ${SRC_DIR}/ImGui/imgui_tables.cpp
    ${SRC_DIR}/ImGui/imgui_widgets.cpp
    ${SRC_DIR}/GUI/Tema/TemaEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/TemaEditorTests.cpp
)
target_include_directories(tema-tests PRIVATE ${SRC_DIR} ${SRC_DIR}/ImGui)
set_target_properties(tema-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME tema-tests COMMAND tema-tests)

if(WIN32)
    target_link_libraries(manifiesto-assets-tests PRIVATE dbghelp)
    target_link_libraries(orquestador-estado-tests PRIVATE dbghelp)
    target_link_libraries(tema-tests PRIVATE dbghelp)
    target_compile_definitions(manifiesto-assets-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(orquestador-estado-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(tema-tests PRIVATE _HAS_STD_BYTE=0)
endif()

endif()

# --- Pruebas headless del subsistema FileManager (sin pila gráfica) ---------
# Compilan en cualquier plataforma (solo std C++17 + los headers de ImGui para
# TreeGUI.h) y ejercitan GestorDeArchivos/FileManager/FileSystemWatcher contra
# un proyecto temporal, sin ventanas. Registradas en ctest.
enable_testing()
add_executable(filemanager-tests
    ${SRC_DIR}/GestorDeArchivos/GestorDeArchivos.cpp
    ${SRC_DIR}/GestorDeArchivos/File.cpp
    ${SRC_DIR}/GestorDeArchivos/Carpeta.cpp
    ${SRC_DIR}/FileManager/FileManager.cpp
    ${SRC_DIR}/FileManager/FileSystemWatcher.cpp
    ${SRC_DIR}/ExcepcionesCPP/RuntimeException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/FileManagerTests.cpp
)
target_include_directories(filemanager-tests PRIVATE ${SRC_DIR}/ImGui)
set_target_properties(filemanager-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME filemanager-tests COMMAND filemanager-tests)

# EditorConfig (JSON tolerante) tambien se prueba headless y en las tres
# plataformas: verifica round-trip y la carga ante ausencia/corrupcion.
add_executable(configuracion-tests
    ${SRC_DIR}/Configuracion/ProjectPaths.cpp
    ${SRC_DIR}/Configuracion/ProjectManager.cpp
    ${SRC_DIR}/Configuracion/ConfigPersistence.cpp
    ${SRC_DIR}/Configuracion/EditorConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/EditorConfigTests.cpp
)
target_include_directories(configuracion-tests PRIVATE ${CMAKE_SOURCE_DIR}/External ${SRC_DIR})
set_target_properties(configuracion-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME configuracion-tests COMMAND configuracion-tests)

# EditorEventBus (canal tipado de GUI interna, Fase 2) se prueba headless como
# los anteriores: subscribe/publish/unsubscribe con payload declarativo, sin
# pila grafica.
add_executable(eventbus-tests
    ${SRC_DIR}/Events/EditorEventBus.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/EditorEventBusTests.cpp
)
target_include_directories(eventbus-tests PRIVATE ${SRC_DIR})
set_target_properties(eventbus-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME eventbus-tests COMMAND eventbus-tests)

# Model (componente de malla) serializa el path con prefijo de longitud. Se
# prueba headless el round-trip y, sobre todo, que un path mas largo que el
# buffer de lectura NO desalinee el stream: regresion del core al cargar
# escenas con paths de mas de 99 bytes (std::length_error -> abort).
add_executable(model-serialization-tests
    ${SRC_DIR}/Configuracion/ProjectPaths.cpp
    ${SRC_DIR}/Configuracion/ProjectManager.cpp
    ${SRC_DIR}/Configuracion/ConfigPersistence.cpp
    ${SRC_DIR}/Objetos/Componentes/Model.cpp
    ${SRC_DIR}/Configuracion/EditorConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/ModelSerializationTests.cpp
)
target_include_directories(model-serialization-tests PRIVATE
    ${SRC_DIR}
    ${CMAKE_SOURCE_DIR}/External
)
set_target_properties(model-serialization-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME model-serialization-tests COMMAND model-serialization-tests)

# MenuModel (logica pura del menu: traduccion, observer de cambios y reset) se
# prueba headless como los demas: no depende de ImGui/GLFW.
add_executable(menu-tests
    ${SRC_DIR}/GUI/MenusGUI/MenuModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/MenuModelTests.cpp
)
target_include_directories(menu-tests PRIVATE ${SRC_DIR})
set_target_properties(menu-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME menu-tests COMMAND menu-tests)

# AssetManager (cache Flyweight de meshes CPU) tambien headless y portatil:
# rutas (AssetPath), geometria (Mesh) y el registro compartido con un loader
# artificial, sin Assimp ni pila grafica.
add_executable(assetmanager-tests
    ${SRC_DIR}/Assets/AssetManager.cpp
    ${SRC_DIR}/Assets/Mesh.cpp
    ${SRC_DIR}/Matematicas/StructVec3.cpp
    ${SRC_DIR}/ExcepcionesCPP/RuntimeException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/AssetManagerTests.cpp
)
target_include_directories(assetmanager-tests PRIVATE ${SRC_DIR})
set_target_properties(assetmanager-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME assetmanager-tests COMMAND assetmanager-tests)

# TextureManager (cache Flyweight de imagenes CPU) headless y portatil al
# igual que assetmanager-tests: registro compartido + loader artificial, sin
# stb_image (StbImageLoader es del engine, aqui no entra la pila grafica).
add_executable(texturemanager-tests
    ${SRC_DIR}/Assets/TextureManager.cpp
    ${SRC_DIR}/ExcepcionesCPP/RuntimeException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/TextureManagerTests.cpp
)
target_include_directories(texturemanager-tests PRIVATE ${SRC_DIR})
set_target_properties(texturemanager-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME texturemanager-tests COMMAND texturemanager-tests)

# Estructuras de datos (headless y portatil): las estructuras son cabeceras
# puras, solo se necesita RuntimeException.cpp por la jerarquia de excepciones.
# Cubre las clases verificadas/corregidas (ListaDE, ArbolEnlazado) y las
# implementadas para el motor (Heaps, ListMergeSort, ArbolBinarioEnlazado).
add_executable(estructuras-tests
    ${SRC_DIR}/ExcepcionesCPP/RuntimeException.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/EstructurasTests.cpp
)
set_target_properties(estructuras-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME estructuras-tests COMMAND estructuras-tests)

# Reflexion de comportamiento (motor de SerializeField): headless y portatil.
# Cubre las macros REFLECT_*, la lectura/escritura de campos (incl. grupos
# anidados y vectores de grupos) y el round-trip binario del arbol de valores.
add_executable(scripts-tests
    ${SRC_DIR}/Behaviour/Reflection/BehaviourReflection.cpp
    ${SRC_DIR}/Matematicas/StructVec3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/ScriptsTests.cpp
)
set_target_properties(scripts-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME scripts-tests COMMAND scripts-tests)

# Runtime de scripts: valida de punta a punta el BackendCpp (compila el fuente
# de un script a .so, lo carga con dlopen y ejecuta el ciclo). Requiere un
# compilador C++ en PATH; si no hay, el test sale con SKIP (77).
# En Windows el test queda ignorado: cl necesita el entorno de Visual Studio.
add_executable(scripts-runtime-tests
    ${SRC_DIR}/Behaviour/Reflection/BehaviourReflection.cpp
    ${SRC_DIR}/Behaviour/Backends/BackendCpp.cpp
    ${SRC_DIR}/Behaviour/ScriptRuntime.cpp
    ${SRC_DIR}/Matematicas/StructVec3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/ScriptsRuntimeTests.cpp
)
set_target_properties(scripts-runtime-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
# The SRC_DIR/CXX_COMPILER macros normally come from the engine target; expose
# them here too para que el BackendCpp pueda compilar el script de prueba.
target_include_directories(scripts-runtime-tests PRIVATE ${SRC_DIR})
target_compile_definitions(scripts-runtime-tests
    PRIVATE FUNSHI_SRC_DIR="${SRC_DIR}"
            FUNSHI_CXX_COMPILER="${CMAKE_CXX_COMPILER}")
# dlopen/dlsym on Linux/macOS requiere -ldl (en glibc moderno esta en libc,
# pero -ldl no molesta); sin backend C fallaria con dlopen.
if(UNIX)
    target_link_libraries(scripts-runtime-tests PRIVATE ${CMAKE_DL_LIBS})
endif()
find_package(Threads QUIET)
if(Threads_FOUND)
    target_link_libraries(scripts-runtime-tests PRIVATE Threads::Threads)
endif()
add_test(NAME scripts-runtime-tests COMMAND scripts-runtime-tests)
# El test devuelve 77 cuando no hay compilador C++ (p. ej. Windows sin cl.exe
# en PATH): ctest lo reporta como SKIP en lugar de fallo.
set_tests_properties(scripts-runtime-tests PROPERTIES SKIP_RETURN_CODE 77)

# AudioEngine (facade de audio, hilo de trabajo) headless y portatil: se monta
# con NullAudioBackend (sin dispositivo) y se prueba el contrato de la cola de
# comandos: registro de clips, handles, encolado no bloqueante, detencion,
# volumen y limpieza. Necesita hilos (El worker thread del motor).
add_executable(audio-tests
    ${SRC_DIR}/Audio/AudioEngine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/AudioEngineTests.cpp
)
target_include_directories(audio-tests PRIVATE ${SRC_DIR})
set_target_properties(audio-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
find_package(Threads QUIET)
if(Threads_FOUND)
    target_link_libraries(audio-tests PRIVATE Threads::Threads)
endif()
add_test(NAME audio-tests COMMAND audio-tests)

# UserInterfaceCustom (asset de interfaces de usuario) headless y portatil:
# modelo puro, solo std C++17 + nlohmann/json vendoriado. Cubre el round-trip
# JSON de los 5 tipos de widget, la persistencia guardar/cargar en un proyecto
# temporal y la tolerancia a JSON parcial.
add_executable(userinterface-tests
    ${SRC_DIR}/GUI/CreadorUI/UserInterfaceCustom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../tests/UserInterfaceTests.cpp
)
target_include_directories(userinterface-tests PRIVATE ${SRC_DIR} ${CMAKE_SOURCE_DIR}/External)
set_target_properties(userinterface-tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
add_test(NAME userinterface-tests COMMAND userinterface-tests)

# Prueba end-to-end del backend Java (se compila solo con FUNSHI_JAVA=ON).
if(FUNSHI_JAVA)
    find_package(JNI REQUIRED)
    find_package(Java QUIET COMPONENTS Development)
    add_executable(scripts-java-tests
        ${SRC_DIR}/Behaviour/Reflection/BehaviourReflection.cpp
        ${SRC_DIR}/Behaviour/Backends/BackendCpp.cpp
        ${SRC_DIR}/Behaviour/Backends/BackendJava.cpp
        ${SRC_DIR}/Behaviour/ScriptRuntime.cpp
        ${SRC_DIR}/Matematicas/StructVec3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../tests/ScriptsJavaTests.cpp
    )
    set_target_properties(scripts-java-tests PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_include_directories(scripts-java-tests
        PRIVATE ${SRC_DIR} ${JNI_INCLUDE_DIRS})
    target_compile_definitions(scripts-java-tests PRIVATE FUNSHI_JAVA)
    if(Java_JAVAC_EXECUTABLE)
        target_compile_definitions(scripts-java-tests
            PRIVATE FUNSHI_JAVAC_DEFAULT="${Java_JAVAC_EXECUTABLE}")
    endif()
    if(JAVA_JVM_LIBRARY)
        target_compile_definitions(scripts-java-tests
            PRIVATE FUNSHI_LIBJVM_DEFAULT="${JAVA_JVM_LIBRARY}")
    endif()
    if(UNIX)
        target_link_libraries(scripts-java-tests PRIVATE ${CMAKE_DL_LIBS})
    endif()
    find_package(Threads QUIET)
    if(Threads_FOUND)
        target_link_libraries(scripts-java-tests PRIVATE Threads::Threads)
    endif()
    add_test(NAME scripts-java-tests COMMAND scripts-java-tests)
    set_tests_properties(scripts-java-tests PROPERTIES SKIP_RETURN_CODE 77)
endif()

if(WIN32)
    # dbghelp: CapturaBackTrace/SymFromAddr/SymInitialize (RuntimeException).
    # En MinGW los #pragma comment(lib) de MSVC se ignoran, hay que enlazarla.
    target_link_libraries(filemanager-tests PRIVATE dbghelp)
    target_link_libraries(assetmanager-tests PRIVATE dbghelp)
    target_link_libraries(texturemanager-tests PRIVATE dbghelp)
    target_link_libraries(estructuras-tests PRIVATE dbghelp)
    target_link_libraries(configuracion-tests PRIVATE dbghelp)
    target_link_libraries(eventbus-tests PRIVATE dbghelp)
    target_link_libraries(model-serialization-tests PRIVATE dbghelp)
    target_link_libraries(menu-tests PRIVATE dbghelp)
    target_link_libraries(scripts-tests PRIVATE dbghelp)
    target_link_libraries(scripts-runtime-tests PRIVATE dbghelp)
    target_link_libraries(audio-tests PRIVATE dbghelp)
    target_link_libraries(userinterface-tests PRIVATE dbghelp)
    # Evitar conflicto std::byte (C++17) vs typedef byte en headers Windows (rpcndr.h, etc.)
    target_compile_definitions(filemanager-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(assetmanager-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(texturemanager-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(estructuras-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(configuracion-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(eventbus-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(model-serialization-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(menu-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(scripts-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(scripts-runtime-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(audio-tests PRIVATE _HAS_STD_BYTE=0)
    target_compile_definitions(userinterface-tests PRIVATE _HAS_STD_BYTE=0)
endif()
