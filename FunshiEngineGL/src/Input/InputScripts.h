#ifndef INPUT_SCRIPTS_H
#define INPUT_SCRIPTS_H

#include <string>

// Consulta de teclado para SCRIPTS (runtime del juego), independiente del
// EditorInput del editor. Mientras la escena esta en Play, main alimenta este
// modulo desde la callback de GLFW (InputScripts::onKey); los scripts consultan
// por NOMBRE de tecla GLFW ("W", "SPACE", "LEFT_SHIFT", "D1", etc.).
//
// Estado: por tecla guarda sostenida (down) y dos flags de edge que
// avanzarFrame() consume: presionada/soltada SOLO retornan true en el frame
// siguiente al evento. GameScene (o main) llama avanzarFrame() una vez por
// frame de juego para rotar el buffer.
//
// Modulo deliberadamente sin GLFW en la interfaz: la traduccion key-code ->
// nombre vive en onKey, y el modulo de scripts no enlaza contra GLFW.
class InputScripts {
public:
    // Recibe el evento de teclado (codigo GLFW + action 0/1/2). Solo
    // registra; la consulta es via sostiene/presionada/soltada.
    void onKey(int key, int action);

    // Rota los edges: presionada()/soltada() dejan de retornar true hasta el
    // proximo evento. Llamar una vez por frame de juego.
    void avanzarFrame();

    // Consulta por nombre de tecla GLFW (sin prefijo "GLFW_KEY_"). Acepta
    // variantes de una letra ("W") o nombre completo ("SPACE", "LEFT_SHIFT").
    bool sostiene(const std::string& tecla) const;
    bool presionada(const std::string& tecla) const;
    bool soltada(const std::string& tecla) const;

    // Traduce un nombre de tecla GLFW a su codigo. Devuelve -1 si no es
    // reconocida (sin lanzar: los scripts consultan con cadenas de usuario).
    static int codigoDe(const std::string& tecla);

    // Limpia todo el estado (al salir de Play).
    void reset();
};

#endif // INPUT_SCRIPTS_H
