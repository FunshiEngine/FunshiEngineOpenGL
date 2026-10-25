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
#include "UIComponent.h"

#include "../../Configuracion/EditorConfig.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

// Marca del formato del payload: va como primer size_t. Un payload de otro
// componente nunca empieza con este valor, asi que el lector sabe de inmediato
// que lo que tiene delante es una interfaz nativa y no otra cosa.
constexpr std::size_t MARCA_UI = 0x55494A31u; // "UIJ1"

// Version del formato. Se incrementa si se anade un campo; las versiones
// anteriores se siguen leyendo (los campos nuevos se quedan en su valor por
// defecto).
constexpr std::uint8_t VERSION_UI = 1;

// Cotas de sanidad: acotan las lecturas para que un stream corrupto o
// desalineado no construya estructuras disparatadas ni desplace la lectura del
// resto del objeto.
constexpr std::size_t COTA_ID = 256;
constexpr std::size_t COTA_TEXTO = 8192;
constexpr std::size_t COTA_RUTA = 4096;
constexpr std::size_t COTA_WIDGETS = 4096;
// Tope absoluto de una cadena en el binario: por encima de esto el archivo se
// considera corrupto y no se intenta ni leer ni saltar.
constexpr std::size_t COTA_BYTES = 16u * 1024u * 1024u;
// Tope del bloque de widgets (en bytes).
constexpr std::size_t COTA_PAYLOAD = 4u * 1024u * 1024u;

void escribirString(std::ostream* out, const std::string& valor) {
	const std::size_t len = valor.size();
	out->write(reinterpret_cast<const char*>(&len), sizeof(len));
	if (len > 0)
		out->write(valor.data(), static_cast<std::streamsize>(len));
}

// Lee un string con una cota. Si el tamano anunciado supera la cota se leen
// solo los bytes permitidos y se salta el resto con seekg: el stream queda al
// final del string y lo que venga despues no se desalinea. Devuelve false si
// el valor hubo que recortarlo o si el stream quedo en error.
bool leerStringAcotado(std::ifstream* file, std::string* destino,
                       std::size_t cota, std::size_t* consumidos) {
	std::size_t len = 0;
	file->read(reinterpret_cast<char*>(&len), sizeof(len));
	*consumidos += sizeof(len);
	if (!file->good()) return false;

	if (len > COTA_BYTES) {
		// Archivo corrupto: no se intenta seguir leyendo.
		file->setstate(std::ios::failbit);
		return false;
	}

	if (len > cota) {
		destino->assign(cota, '\0');
		file->read(destino->data(), static_cast<std::streamsize>(cota));
		const std::size_t restantes = len - cota;
		file->seekg(static_cast<std::streamoff>(restantes), std::ios::cur);
		*consumidos += len;
		return false;
	}

	destino->assign(len, '\0');
	if (len > 0)
		file->read(destino->data(), static_cast<std::streamsize>(len));
	*consumidos += len;
	return file->good();
}

void escribirWidget(std::ostream* out, const WidgetUI& widget) {
	escribirString(out, widget.id);

	const std::uint8_t tipo = static_cast<std::uint8_t>(widget.tipo);
	out->write(reinterpret_cast<const char*>(&tipo), sizeof(tipo));
	const std::uint8_t ancla = static_cast<std::uint8_t>(widget.ancla);
	out->write(reinterpret_cast<const char*>(&ancla), sizeof(ancla));

	out->write(reinterpret_cast<const char*>(&widget.desplazamiento.x),
	           sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.desplazamiento.y),
	           sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.tamano.x), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.tamano.y), sizeof(float));

	out->write(reinterpret_cast<const char*>(&widget.color.r), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.color.g), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.color.b), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.color.a), sizeof(float));

	out->write(reinterpret_cast<const char*>(&widget.colorTexto.r), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.colorTexto.g), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.colorTexto.b), sizeof(float));
	out->write(reinterpret_cast<const char*>(&widget.colorTexto.a), sizeof(float));

	escribirString(out, widget.texto);
	escribirString(out, widget.rutaTextura);

	out->write(reinterpret_cast<const char*>(&widget.tamanoTexto), sizeof(float));

	const std::uint8_t visible = widget.visible ? 1 : 0;
	out->write(reinterpret_cast<const char*>(&visible), sizeof(visible));
}

bool leerWidget(std::ifstream* file, WidgetUI* widget, std::size_t* consumidos) {
	if (!leerStringAcotado(file, &widget->id, COTA_ID, consumidos)) return false;

	std::uint8_t tipo = 0;
	file->read(reinterpret_cast<char*>(&tipo), sizeof(tipo));
	*consumidos += sizeof(tipo);
	if (!file->good()) return false;
	if (tipo <= static_cast<std::uint8_t>(TipoWidgetUI::Boton))
		widget->tipo = static_cast<TipoWidgetUI>(tipo);
	else
		widget->tipo = TipoWidgetUI::Etiqueta;

	std::uint8_t ancla = 0;
	file->read(reinterpret_cast<char*>(&ancla), sizeof(ancla));
	*consumidos += sizeof(ancla);
	if (!file->good()) return false;
	if (ancla < static_cast<std::uint8_t>(kTotalAnclasUI))
		widget->ancla = static_cast<AnclaUI>(ancla);
	else
		widget->ancla = AnclaUI::ArribaIzquierda;

	file->read(reinterpret_cast<char*>(&widget->desplazamiento.x), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->desplazamiento.y), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->tamano.x), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->tamano.y), sizeof(float));
	*consumidos += 4 * sizeof(float);

	file->read(reinterpret_cast<char*>(&widget->color.r), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->color.g), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->color.b), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->color.a), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->colorTexto.r), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->colorTexto.g), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->colorTexto.b), sizeof(float));
	file->read(reinterpret_cast<char*>(&widget->colorTexto.a), sizeof(float));
	*consumidos += 8 * sizeof(float);

	if (!leerStringAcotado(file, &widget->texto, COTA_TEXTO, consumidos))
		return false;
	std::string ruta;
	if (!leerStringAcotado(file, &ruta, COTA_RUTA, consumidos)) return false;
	// La ruta de la textura se guarda relativa al proyecto y se vuelve
	// absoluta al cargar, como en el resto de los componentes con archivos.
	widget->rutaTextura = EditorConfig::absolutizarRuta(ruta);

	file->read(reinterpret_cast<char*>(&widget->tamanoTexto), sizeof(float));
	*consumidos += sizeof(float);

	std::uint8_t visible = 1;
	file->read(reinterpret_cast<char*>(&visible), sizeof(visible));
	*consumidos += sizeof(visible);
	widget->visible = visible != 0;

	return file->good();
}

} // namespace

void UIComponent::serializeComponent(std::ofstream* file) {
	if (!file || !file->is_open()) return;

	const std::size_t marca = MARCA_UI;
	file->write(reinterpret_cast<const char*>(&marca), sizeof(marca));

	const std::uint8_t version = VERSION_UI;
	file->write(reinterpret_cast<const char*>(&version), sizeof(version));

	const std::uint8_t activoByte = activo ? 1 : 0;
	file->write(reinterpret_cast<const char*>(&activoByte), sizeof(activoByte));

	file->write(reinterpret_cast<const char*>(&z), sizeof(float));

	// El bloque de widgets se arma aparte para poder escribir delante su tamano
	// en bytes: asi el lector sabe cuanto ocupa y no se desalinea aunque los
	// datos esten corruptos.
	std::ostringstream bloque(std::ios::binary);
	const std::size_t cantidad = lienzo_.widgets().size();
	bloque.write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
	for (const WidgetUI& widget : lienzo_.widgets()) {
		WidgetUI copia = widget;
		copia.rutaTextura = EditorConfig::relativizarRuta(copia.rutaTextura);
		escribirWidget(&bloque, copia);
	}
	const std::string datos = bloque.str();
	const std::size_t tamano = datos.size();
	file->write(reinterpret_cast<const char*>(&tamano), sizeof(tamano));
	if (tamano > 0)
		file->write(datos.data(), static_cast<std::streamsize>(tamano));
}

void UIComponent::deserializeComponent(std::ifstream* file) {
	if (!file || !file->is_open()) return;

	std::size_t marca = 0;
	file->read(reinterpret_cast<char*>(&marca), sizeof(marca));
	if (!file->good()) return;
	if (marca != MARCA_UI) {
		// No es el payload de una interfaz nativa: no se consume nada mas para
		// no desalinear el resto del objeto.
		return;
	}

	std::uint8_t version = VERSION_UI;
	file->read(reinterpret_cast<char*>(&version), sizeof(version));
	if (!file->good()) return;

	std::uint8_t activoByte = 1;
	file->read(reinterpret_cast<char*>(&activoByte), sizeof(activoByte));
	activo = activoByte != 0;

	file->read(reinterpret_cast<char*>(&z), sizeof(float));

	std::size_t tamano = 0;
	file->read(reinterpret_cast<char*>(&tamano), sizeof(tamano));
	if (!file->good()) return;
	if (tamano > COTA_PAYLOAD) {
		file->setstate(std::ios::failbit);
		return;
	}

	// A partir de aqui el bloque de widgets tiene un tamano conocido: si algo
	// se sale de las cotas, se salta el resto del bloque y el stream queda
	// alineado para el siguiente componente.
	std::size_t consumidos = 0;

	std::size_t cantidad = 0;
	file->read(reinterpret_cast<char*>(&cantidad), sizeof(cantidad));
	consumidos += sizeof(cantidad);
	if (!file->good()) return;

	if (cantidad > COTA_WIDGETS) {
		// Cantidad disparatada: se salta el bloque entero.
		file->seekg(static_cast<std::streamoff>(tamano - consumidos),
		            std::ios::cur);
		return;
	}

	std::vector<WidgetUI> widgets;
	widgets.reserve(cantidad);
	for (std::size_t i = 0; i < cantidad; ++i) {
		WidgetUI widget;
		if (!leerWidget(file, &widget, &consumidos)) {
			// El bloque se termino antes de tiempo: se salta lo que falte.
			if (consumidos < tamano)
				file->seekg(static_cast<std::streamoff>(tamano - consumidos),
				            std::ios::cur);
			break;
		}
		widgets.push_back(std::move(widget));
	}

	// El estado vivo de la interaccion se reconstruye: no viene del archivo.
	sistema_ = SistemaUI();
	lienzo_ = LienzoUI();
	for (const WidgetUI& widget : widgets) {
		lienzo_.agregar(widget);
	}
}

void UIComponent::actualizarInteraccion(const EntradaUI& entrada,
                                        float anchoPantalla,
                                        float altoPantalla) {
	lienzo_.layout(anchoPantalla, altoPantalla);
	sistema_.actualizar(lienzo_, entrada);
}
