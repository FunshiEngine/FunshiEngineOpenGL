// FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
// Copyright 2026 Gianfranco Ivan Enrique
// Licensed under the Apache License, Version 2.0 (SPDX-License-Identifier: Apache-2.0)
//
// TODOS los textos del motor, en bloques por idioma (formato posicional).
//
// - Cada bloque empieza con IDIOMA(numero, "codigo", "nombre") y los numeros
//   van en orden: 0, 1, 2...
// - Todos los bloques tienen los mismos N textos en el MISMO orden. N es la
//   cantidad de TEXTO(...) de un bloque (Localizacion::kCantidadTextos).
// - Posicion: el texto i del idioma n esta en la posicion n * N + i de la
//   tabla. El nombre de TEXTO(Nombre, ...) solo sirve para que el codigo diga
//   Texto::Nombre en lugar de un numero suelto, y para que el compilador
//   compruebe que cada bloque tiene los textos completos y en el mismo orden.
// - Un texto vacio ("") en un idioma muestra el del idioma 0 (espanol).
//
// Agregar un texto: una linea TEXTO en CADA bloque, en la misma posicion.
// Agregar un idioma: copiar el bloque 0 al final, cambiar el numero de
// TEXTOS_BLOQUE e IDIOMA y traducir los textos.
//
// Este archivo es de datos: no tiene limite de lineas. Lo incluye
// Localizacion.h/.cpp definiendo TEXTOS_BLOQUE, IDIOMA y TEXTO antes; no
// incluirlo desde otro lado.

#if TEXTOS_BLOQUE < 0 || TEXTOS_BLOQUE == 0
IDIOMA(0, "es", "Espanol")
TEXTO(IniciarEstudio, "Iniciar Estudio")
TEXTO(Proyectos, "Proyectos")
TEXTO(SinProyectos, "(no hay proyectos: crea uno en Configurar proyecto)")
TEXTO(ConfigProyecto, "Configurar proyecto")
TEXTO(CrearProyecto, "Crear proyecto")
TEXTO(ImportarProyecto, "Importar proyecto")
TEXTO(ImportarNoDisponible,
      "Importar un proyecto todavia no esta disponible.")
TEXTO(ProyectoActual, "Proyecto actual")
TEXTO(Dimension, "Tipo de proyecto")
TEXTO(Dimension2D, "2D - Plano")
TEXTO(Descripcion2D,
      "Para juegos vistos de frente, como una hoja de papel: los objetos se "
      "mueven a lo ancho y a lo alto. Ideal para plataformas, puzzles y "
      "juegos arcade.")
TEXTO(Dimension3D, "3D - Profundidad")
TEXTO(Descripcion3D,
      "Para mundos con profundidad, como el mundo real: los objetos se "
      "mueven a lo ancho, a lo alto y hacia adentro. Ideal para "
      "exploracion, accion y mundos abiertos.")
TEXTO(AvisoNoRevertir,
      "Una vez creado, el tipo de proyecto no se puede cambiar: no se puede "
      "pasar de 2D a 3D ni al reves. Elegi con cuidado.")
TEXTO(Crear, "Crear")
TEXTO(Cancelar, "Cancelar")
TEXTO(NombreVacio, "Escribi un nombre para el proyecto.")
TEXTO(NombreRepetido, "Ya existe un proyecto con ese nombre.")
TEXTO(Opciones, "Opciones")
TEXTO(Configuracion, "Configuracion")
TEXTO(Salir, "Salir")
TEXTO(Volver, "Volver")
TEXTO(Juego, "Juego")
TEXTO(Idioma, "Idioma")
TEXTO(RadioDifuminado, "Radio de difuminado")
TEXTO(SensibilidadCamara, "Sensibilidad de camara")
TEXTO(SensibilidadMovimiento, "Sensibilidad de movimiento")
TEXTO(Apariencia, "Apariencia")
TEXTO(Audio, "Audio")
TEXTO(Controles, "Controles")
TEXTO(VolumenMaestro, "Volumen maestro")
TEXTO(TemaClaro, "Tema claro de la interfaz")
TEXTO(ModoBN, "Modo blanco y negro (interfaz y viewport)")
TEXTO(ColorAcento, "Color de acento de la interfaz")
TEXTO(ColorCieloSuperior, "Color de la parte superior del cielo")
TEXTO(ColorCieloInferior, "Color de la parte inferior del cielo")
TEXTO(AyudaBN,
      "El modo blanco y negro desatura toda la interfaz (incluido el\n"
      "acento) y usa blanco/negro en el fondo y la grilla del viewport\n"
      "segun el tema claro u oscuro.")
TEXTO(RestablecerApariencia, "Restablecer apariencia")
TEXTO(RestablecerConfiguracion, "Restablecer configuracion")
TEXTO(AyudaReset,
      "Reinicia idioma, apariencia, sensibilidades y estado del editor a\n"
      "los valores de fabrica.")
TEXTO(Nombre, "Nombre")
TEXTO(EditarNombre, "Editar nombre")
TEXTO(EliminarProyecto, "Eliminar proyecto")
TEXTO(AvisoEliminar,
      "Se eliminaran de disco la escena, los assets y la configuracion\n"
      "de este proyecto. Esta accion no se puede deshacer.")
TEXTO(Eliminar, "Eliminar")
TEXTO(Confirmar, "Confirmar")
TEXTO(Actual, "(actual: %s)")
TEXTO(Archivo, "Archivo")
TEXTO(ExportarJuego, "Exportar juego")
TEXTO(Ventanas, "Ventanas")
TEXTO(PanelesDeMods, "Paneles de mods")
TEXTO(Gizmo, "Gizmo")
TEXTO(LocalEjesDelObjeto, "Local (ejes del objeto)")
TEXTO(GlobalEjesDelMundo, "Global (ejes del mundo)")
TEXTO(LaTeclaGAlternaEntre,
      "La tecla G alterna entre ambos.\nOperacion: 1/T mover, 2/R rotar, 3/Y escalar")
TEXTO(Estado, "Estado: %s%s")
TEXTO(ID, "ID: %d")
TEXTO(CambiarID, "Cambiar ID")
TEXTO(Renombrar, "Renombrar")
TEXTO(DesanidarARaiz, "Desanidar a raiz")
TEXTO(CrearPrefab, "Crear Prefab")
TEXTO(Moviendo, "Moviendo %s")
TEXTO(DialogoCambiarID, "DialogoCambiarID")
TEXTO(NuevoIDPara, "Nuevo ID para '%s':")
TEXTO(Aceptar, "Aceptar")
TEXTO(DialogoRenombrar, "DialogoRenombrar")
TEXTO(NuevoNombre, "Nuevo nombre:")
TEXTO(DialogoEliminar, "DialogoEliminar")
TEXTO(EliminarID, "Eliminar '%s' (ID: %d)?")
TEXTO(EstaAccionNoSePuede, "Esta accion no se puede deshacer.")
TEXTO(DialogoCrearPrefab, "DialogoCrearPrefab")
TEXTO(NombreDelPrefab, "Nombre del prefab:")
TEXTO(NewSimpleObject, "New SimpleObject")
TEXTO(EliminarCarpeta, "Eliminar Carpeta")
TEXTO(EliminarArchivo, "Eliminar Archivo")
TEXTO(SoltarEnZonaDeDestino, "Soltar en zona de destino")
TEXTO(CopiarDentroDe, "Copiar dentro de %s")
TEXTO(MoverACtrlCopiar, "Mover a %s  (Ctrl = copiar)")
TEXTO(Tipo, "%s\nTipo: %s")
TEXTO(SubirACarpetaPadre, "Subir a carpeta padre")
TEXTO(NewScript, "New Script")
TEXTO(NewFolder, "New Folder")
TEXTO(NewFile, "New File")
TEXTO(CopyExistFolder, "Copy Exist Folder")
TEXTO(CopyExistFile, "Copy Exist File")
TEXTO(IngresarNombre, "Ingresar nombre")
TEXTO(SeleccionarTipoDeScript, "Seleccionar Tipo de Script")
TEXTO(SeleccionaElTipoDeScript, "Selecciona el tipo de script a crear:")
TEXTO(Continuar, "Continuar")
TEXTO(ConfirmarEliminarArchivo, "ConfirmarEliminarArchivo")
TEXTO(EliminarArchivo2, "Eliminar archivo \"%s\"?")
TEXTO(ConfirmarEliminarCarpetaGrid, "ConfirmarEliminarCarpetaGrid")
TEXTO(EliminarCarpetaYTodoSu, "Eliminar carpeta \"%s\" y todo su contenido?")
TEXTO(MoviendoCarpeta, "Moviendo carpeta %s")
TEXTO(MoverA, "Mover a %s")
TEXTO(Carpeta, "Carpeta: %s")
TEXTO(RenombrarCarpeta, "Renombrar Carpeta")
TEXTO(NuevaCarpeta, "Nueva Carpeta")
TEXTO(ConfirmarEliminar, "ConfirmarEliminar")
TEXTO(EliminarYTodoSuContenido, "Eliminar \"%s\" y todo su contenido?")
TEXTO(EscribeElNombreDeLa, "Escribe el nombre de la carpeta:")
TEXTO(Raiz, "(raiz)")
TEXTO(CopiarEnLaCarpetaActual, "Copiar en la carpeta actual")
TEXTO(MoverALaCarpetaActual, "Mover a la carpeta actual  (Ctrl = copiar)")
TEXTO(EscribeNombreScriptJava, "Escribe el nombre del script Java:")
TEXTO(EscribeNombreScriptCpp, "Escribe el nombre del script C++:")
TEXTO(EscribeNombreArchivo, "Escribe el nombre del archivo:")
TEXTO(NuevoNombreCarpeta, "Nuevo nombre de la carpeta:")
TEXTO(NuevoNombreArchivo, "Nuevo nombre del archivo:")
#endif

#if TEXTOS_BLOQUE < 0 || TEXTOS_BLOQUE == 1
IDIOMA(1, "en", "English")
TEXTO(IniciarEstudio, "Start Studio")
TEXTO(Proyectos, "Projects")
TEXTO(SinProyectos, "(no projects: create one in Configure project)")
TEXTO(ConfigProyecto, "Configure project")
TEXTO(CrearProyecto, "Create project")
TEXTO(ImportarProyecto, "Import project")
TEXTO(ImportarNoDisponible, "Importing a project is not available yet.")
TEXTO(ProyectoActual, "Current project")
TEXTO(Dimension, "Project type")
TEXTO(Dimension2D, "2D - Flat")
TEXTO(Descripcion2D,
      "For games seen from the front, like a sheet of paper: objects move "
      "across and up. Great for platformers, puzzles and arcade games.")
TEXTO(Dimension3D, "3D - Depth")
TEXTO(Descripcion3D,
      "For worlds with depth, like the real world: objects move across, up "
      "and into the scene. Great for exploration, action and open worlds.")
TEXTO(AvisoNoRevertir,
      "Once created, the project type cannot be changed: you cannot switch "
      "from 2D to 3D or back. Choose carefully.")
TEXTO(Crear, "Create")
TEXTO(Cancelar, "Cancel")
TEXTO(NombreVacio, "Type a name for the project.")
TEXTO(NombreRepetido, "A project with that name already exists.")
TEXTO(Opciones, "Options")
TEXTO(Configuracion, "Settings")
TEXTO(Salir, "Exit")
TEXTO(Volver, "Back")
TEXTO(Juego, "Game")
TEXTO(Idioma, "Language")
TEXTO(RadioDifuminado, "Fade radius")
TEXTO(SensibilidadCamara, "Camera sensitivity")
TEXTO(SensibilidadMovimiento, "Movement sensitivity")
TEXTO(Apariencia, "Appearance")
TEXTO(Audio, "Audio")
TEXTO(Controles, "Controls")
TEXTO(VolumenMaestro, "Master volume")
TEXTO(TemaClaro, "Light UI theme")
TEXTO(ModoBN, "Black & white mode (UI and viewport)")
TEXTO(ColorAcento, "UI accent color")
TEXTO(ColorCieloSuperior, "Sky top color")
TEXTO(ColorCieloInferior, "Sky bottom color")
TEXTO(AyudaBN,
      "Black & white mode desaturates the whole UI (accent included) and\n"
      "uses black/white for the background and viewport grid according\n"
      "to the light or dark theme.")
TEXTO(RestablecerApariencia, "Reset appearance")
TEXTO(RestablecerConfiguracion, "Reset configuration")
TEXTO(AyudaReset,
      "Resets language, appearance, sensitivities and editor\n"
      "state to factory defaults.")
TEXTO(Nombre, "Name")
TEXTO(EditarNombre, "Edit name")
TEXTO(EliminarProyecto, "Delete project")
TEXTO(AvisoEliminar,
      "The scene, assets and settings of this project will be removed\n"
      "from disk. This action cannot be undone.")
TEXTO(Eliminar, "Delete")
TEXTO(Confirmar, "Confirm")
TEXTO(Actual, "(current: %s)")
TEXTO(Archivo, "File")
TEXTO(ExportarJuego, "Export game")
TEXTO(Ventanas, "Windows")
TEXTO(PanelesDeMods, "Mod panels")
TEXTO(Gizmo, "Gizmo")
TEXTO(LocalEjesDelObjeto, "Local (object axes)")
TEXTO(GlobalEjesDelMundo, "Global (world axes)")
TEXTO(LaTeclaGAlternaEntre,
      "The G key toggles between both.\nOperation: 1/T move, 2/R rotate, 3/Y scale")
TEXTO(Estado, "Status: %s%s")
TEXTO(ID, "ID: %d")
TEXTO(CambiarID, "Change ID")
TEXTO(Renombrar, "Rename")
TEXTO(DesanidarARaiz, "Unparent to root")
TEXTO(CrearPrefab, "Create Prefab")
TEXTO(Moviendo, "Moving %s")
TEXTO(DialogoCambiarID, "Change ID")
TEXTO(NuevoIDPara, "New ID for '%s':")
TEXTO(Aceptar, "Accept")
TEXTO(DialogoRenombrar, "Rename")
TEXTO(NuevoNombre, "New name:")
TEXTO(DialogoEliminar, "Delete")
TEXTO(EliminarID, "Delete '%s' (ID: %d)?")
TEXTO(EstaAccionNoSePuede, "This action cannot be undone.")
TEXTO(DialogoCrearPrefab, "Create Prefab")
TEXTO(NombreDelPrefab, "Prefab name:")
TEXTO(NewSimpleObject, "New SimpleObject")
TEXTO(EliminarCarpeta, "Delete Folder")
TEXTO(EliminarArchivo, "Delete File")
TEXTO(SoltarEnZonaDeDestino, "Drop on target area")
TEXTO(CopiarDentroDe, "Copy into %s")
TEXTO(MoverACtrlCopiar, "Move to %s  (Ctrl = copy)")
TEXTO(Tipo, "%s\nType: %s")
TEXTO(SubirACarpetaPadre, "Go up to parent folder")
TEXTO(NewScript, "New Script")
TEXTO(NewFolder, "New Folder")
TEXTO(NewFile, "New File")
TEXTO(CopyExistFolder, "Copy Existing Folder")
TEXTO(CopyExistFile, "Copy Existing File")
TEXTO(IngresarNombre, "Enter name")
TEXTO(SeleccionarTipoDeScript, "Select Script Type")
TEXTO(SeleccionaElTipoDeScript, "Select the type of script to create:")
TEXTO(Continuar, "Continue")
TEXTO(ConfirmarEliminarArchivo, "Delete file")
TEXTO(EliminarArchivo2, "Delete file \"%s\"?")
TEXTO(ConfirmarEliminarCarpetaGrid, "Delete folder")
TEXTO(EliminarCarpetaYTodoSu, "Delete folder \"%s\" and all its contents?")
TEXTO(MoviendoCarpeta, "Moving folder %s")
TEXTO(MoverA, "Move to %s")
TEXTO(Carpeta, "Folder: %s")
TEXTO(RenombrarCarpeta, "Rename Folder")
TEXTO(NuevaCarpeta, "New Folder")
TEXTO(ConfirmarEliminar, "Delete")
TEXTO(EliminarYTodoSuContenido, "Delete \"%s\" and all its contents?")
TEXTO(EscribeElNombreDeLa, "Type the folder name:")
TEXTO(Raiz, "(root)")
TEXTO(CopiarEnLaCarpetaActual, "Copy into the current folder")
TEXTO(MoverALaCarpetaActual, "Move to the current folder  (Ctrl = copy)")
TEXTO(EscribeNombreScriptJava, "Type the Java script name:")
TEXTO(EscribeNombreScriptCpp, "Type the C++ script name:")
TEXTO(EscribeNombreArchivo, "Type the file name:")
TEXTO(NuevoNombreCarpeta, "New folder name:")
TEXTO(NuevoNombreArchivo, "New file name:")
#endif
