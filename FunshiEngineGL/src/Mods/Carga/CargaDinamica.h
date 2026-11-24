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

// Carga dinamica multiplataforma (dlopen/LoadLibrary). Extraido de
// Behaviour/Backends/BackendCpp.cpp sin cambiar su comportamiento.

#ifndef CARGA_DINAMICA_H
#define CARGA_DINAMICA_H

#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#include <windows.h>
#define FUNSHI_DLOPEN(name) LoadLibraryA((name).c_str())
#define FUNSHI_DLSYM(handle, symbol) GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol)
#define FUNSHI_DLOPENCERRAR(handle) FreeLibrary(reinterpret_cast<HMODULE>(handle))
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "dll"
#elif defined(__APPLE__)
#include <dlfcn.h>
#define FUNSHI_DLOPEN(name) dlopen((name).c_str(), RTLD_NOW | RTLD_LOCAL)
#define FUNSHI_DLSYM(handle, symbol) dlsym(handle, symbol)
#define FUNSHI_DLOPENCERRAR(handle) dlclose(handle)
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "dylib"
#else
#include <dlfcn.h>
#include <unistd.h>
#define FUNSHI_DLOPEN(name) dlopen((name).c_str(), RTLD_NOW | RTLD_LOCAL)
#define FUNSHI_DLSYM(handle, symbol) dlsym(handle, symbol)
#define FUNSHI_DLOPENCERRAR(handle) dlclose(handle)
#define FUNSHI_SYM_CREAR "FUNSHI_CREAR_COMPORTAMIENTO"
#define FUNSHI_ARTEFACTO_EXT "so"
#endif

#endif // CARGA_DINAMICA_H
