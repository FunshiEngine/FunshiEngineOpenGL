/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    SPDX-License-Identifier: Apache-2.0
*/
#include "SceneSerializationTestsDetalle.h"

int main() {
    nombresPorDefecto();
    roundTripDeEscena();
    tagsCompatiblesConEscenasAnteriores();
    restaurarBaselineDeEscena();
    guardadoConArbolVacio();
    reporteFalloBinario();
    indiceCorruptoSinFantasmas();
    diagnosticosCargaEscena();
    diagnosticosCargaMalla();
    diagnosticosCargaScript();
    hijoConIdCeroSeReasignaAlGuardar();
    hermanosConsecutivosSinPerdida();
    rutasDeCarasDeSkybox();
    reescrituraDeReferencias();
    sanadoDeRutasRotas();
    variosScriptsPorObjeto();
    nombreDeComponenteScriptSeSerializa();
    rotacionApiUsaRadianes();
    elInspectorSeDesvinculaAlBorrar();
    laCamaraBorradaNoSeUsaEnElRestoDelFrame();
    borrarCrearBorrarNoDesalineaElBinario();
    elModeloSeResuelveConLaMatrizMundial();
    elEventoDePropiedadNoReconstruyeElInspector();
    elInspectorDibujaUnFrameSinArrastre();
    reparentarPreservaLaPoseYRefrescaElCuerpo();
    reparentarSobreviveElGuardado();
    opcionesDeRenderYFisicaSePersisten();
    movimientoFisicoYSaltoDesdeScripts();
    eventosDeContactoExponenColliderYPropietario();
    busquedaDeObjetosPorNombreIdYEtiqueta();
    musicaDesdeScripts();
    propiedadesFisicasDesdeScripts();
    rigidBodySerializaPropiedadesNuevas();
    sensorDetectaSinRespuestaYPlano2D();
    gestionDeObjetosDesdeScripts();
    siembraPorDimension();
    proyeccionDeCamara();
    spriteRendererSerializaRoundTrip();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
