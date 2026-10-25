# Plan: Limpiar archivos binarios huérfanos al guardar escena

## Problema
Al guardar una escena, los archivos `ObjectN*.db` huérfanos (de objetos borrados anteriormente) que quedan en `Memory/Binarios/Scene/` no se eliminan. Solo se trunca el índice `SceneBBDDObjetos.txt`, pero los binarios `.db` antiguos permanecen.

La petición indica: limpiar el contenido de esa carpeta antes de escribir, dejando solo la raíz `ObjectN0.db` (nunca perderla), y luego regenerar únicamente los objetos que corresponden al árbol actual.

## Contexto arquitectónico
- **Rutas**: `ProjectPaths::rutaSceneDir(nombreProyecto)` devuelve `<base>/Proyects/<nombre>/Memory/Binarios/Scene/`. `ProjectPaths::rutaScenePrefijo()` devuelve el prefijo `<base>/.../Memory/Binarios/Scene` (sin `/` final). `SceneSerializer::save(const std::string& filename)` recibe ese prefijo y construye `pathTxt = filename + "BBDDObjetos.txt"`. Los binarios se guardan en `filename + "/ObjectN" + id + ".db"`.
- **Guardado**: `SceneSerializer::save()` abre el índice con `std::ios::trunc` (limpia el índice), luego llama a `savePreOrder()` desde la raíz del árbol. `savePreOrder()` guarda cada `GameObject::saveEntity(filename)` y escribe su basename en el índice con la estructura preorden `=>`/`<=`.
- **Carga**: `SceneSerializer::load()` lee el índice y reconstruye el árbol. El directorio donde se buscan los `.db` es `semiPath` (pasado desde `GameScene`).
- **Raíz**: `ObjectN0.db` corresponde al objeto raíz. Nunca debe borrarse. `SceneSerializer` ya protege contra que hijos tengan `id==0` (H-17).
- **Juego/Exportación**: Usa los mismos paths via `ProjectPaths`. Hay que tener cuidado de no afectar exportaciones, pero el guardado desde editor usa los paths del proyecto actual.

## Solución propuesta
Modificar `SceneSerializer::save()` para limpiar la carpeta de binarios antes de guardar.

### Opción elegida (más limpia y segura)
1. **Recolectar IDs a guardar**: Hacer un recorrido previo sobre el árbol actual para obtener todos los IDs de objetos que se van a escribir. Esto evita tener que parsear archivos o adivinar.
2. **Limpiar carpeta**: Antes de truncar el índice, obtener el directorio `Scene/` desde `filename` (o construirlo como `filename + "/"` o usar `ProjectPaths::rutaSceneDir` si fuera posible, pero aquí solo tenemos `filename` prefijo). Recorrer todos los archivos `ObjectN*.db` en ese directorio. Para cada archivo cuyo ID != 0, si ese ID no está en la lista recolectada, borrarlo con `std::filesystem::remove()`.
3. **Guardar normalmente**: Truncar índice, recorrer árbol con `savePreOrder()` y escribir los `.db` que falten (o sobrescribirlos). `savePreOrder()` ya llama a `object->saveEntity(filename)` para cada objeto.

### Detalles de implementación
- **Calcular directorio**: `std::filesystem::path dirBinarios = std::filesystem::path(filename);` si `filename` ya apunta al directorio `Scene/` (ver `ProjectPaths::rutaSceneDir` usa `+ "/Scene/"`, `rutaScenePrefijo` usa `+ "/Scene"`). Mirando `GameScene::guardarEscena()`: usa `ProjectPaths::rutaScenePrefijo(nombreProyecto)` que devuelve `.../Memory/Binarios/Scene` (sin slash final). Y pasa ese prefijo a `serializer->save()`. También `GameScene` construye `indiceSnapshot_ = (candidato / "SceneBBDDObjetos.txt").string()` donde `candidato` es la carpeta. Hay que ser preciso con la ruta.
- **Recolector de IDs**: Crear función auxiliar `void recolectarIds(GameObjectTree* arbol, std::unordered_set<int>& ids)` que recorra todo el árbol (recursivo o iterativo) y añada `object->getId()` de cada nodo.
- **Limpieza**: `std::filesystem::directory_iterator` sobre `dirBinarios`. Filtrar por extensión `.db` y patrón `ObjectN*.db`. Parsear ID con la misma lógica que `idDesdeLinea` o simplemente extraer número. Si ID == 0 saltar. Si no está en el set, `std::filesystem::remove(entry.path())` con aviso por stderr si falla.
- **Orden correcto**: Hacer limpieza ANTES de truncar índice y ANTES de escribir. Así nos aseguramos de que no queden huérfanos aunque falle algo después (aunque lo ideal es hacerlo todo en `save()`).
- **Multiplataforma**: Usar `std::filesystem` (ya se usa en el proyecto). En Windows/Linux funciona.

### Puntos a tener en cuenta
- **Raíz**: `ObjectN0.db` siempre se conserva. Aunque esté en el árbol, también lo mantenemos explícitamente.
- **Árbol vacío**: Si la escena queda vacía (`escenaVacia == true`), hay que limpiar todos los `ObjectN*.db` menos `ObjectN0.db`? O dejar solo la raíz. El comportamiento actual con árbol vacío deja el índice vacío (avisa y trunca). Los `.db` huérfanos quedarían. Con la limpieza, al guardar escena vacía se borrarían todos los demás `.db`, dejando solo `ObjectN0.db` si existe. Eso parece razonable para "limpiar antes de escribir".
- **Permisos/errores**: No debe bloquear el guardado si falla borrar algún archivo. Solo loguear aviso por stderr.
- **Performance**: Árboles pequeños/medianos, directorio con pocos archivos. No problemático.
- **No tocar fuera de Scene/**: Solo limpiar ese directorio concreto.

## Archivos a modificar
- `FunshiEngineGL/src/Scenes/SceneSerializer.h` - Añadir declaración de helper si necesario (puede ser estático/anónimo)
- `FunshiEngineGL/src/Scenes/SceneSerializer.cpp` - Implementar limpieza en `save()`

## Validación
1. **Build**: `cmake --build FunshiEngineGL/build -j$(nproc)` debe compilar sin errores
2. **Tests**: `cd FunshiEngineGL/build && ctest --output-on-failure` debe pasar todos los tests
3. **Prueba manual**: 
   - Crear escena con varios objetos, guardar
   - Borrar algunos objetos, guardar
   - Verificar que los `.db` de objetos borrados ya no existen en `Memory/Binarios/Scene/`
   - Verificar que `ObjectN0.db` sigue existiendo
   - Verificar que `SceneBBDDObjetos.txt` solo lista los objetos actuales
   - Cargar escena y comprobar que funciona correctamente
4. **Test existente**: `tests/SceneSerializationTests.cpp` debe seguir pasando

## Pasos de implementación
1. Leer `SceneSerializer.h` para entender la clase
2. Añadir includes necesarios (`<filesystem>`, `<unordered_set>`, `<vector>` si faltan)
3. Implementar función para recolectar IDs del árbol
4. Implementar función para limpiar directorio
5. Modificar `SceneSerializer::save()` para llamar a limpieza antes de truncar
6. Compilar y testear
7. Documentar cambios si necesario

## Riesgos
- **Ruta incorrecta**: Si `filename` no apunta donde esperamos, podríamos borrar archivos erróneos. Hay que calcular correctamente el directorio.
- **Race conditions**: Poco probable en uso normal del editor
- **Permisos**: En algunos sistemas no se pueden borrar archivos
