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
// windows.h debe ir ANTES de cualquier header de la stdlib: rpcndr.h define un
// typedef global 'byte' que colisiona con std::byte (C++17) ya visible cuando
// la stdlib ya se expandio (<> en MinGW: "reference to 'byte' is ambiguous").
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

#include "RuntimeException.h"
#include <sstream>

#ifndef _WIN32
#include <execinfo.h>
#include <cxxabi.h>
#include <dlfcn.h>
#endif
using namespace std;
RuntimeException::RuntimeException(const string& msg)
    : std::runtime_error(msg),
    stackTrace(generateStackTrace()) {
}

std::string RuntimeException::getStackTrace() const {
    std::ostringstream oss;
    oss << "Exception: " << what() << "\nStack Trace:\n";

    for (const auto& frame : stackTrace) {
        oss << "\tat " << frame << "\n";
    }

    return oss.str();
}

std::vector<std::string> RuntimeException::generateStackTrace() {
    std::vector<std::string> trace;

#ifdef _WIN32
    void* stack[100];
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);
    WORD frames = CaptureStackBackTrace(0, 100, stack, NULL);
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    for (WORD i = 1; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        trace.push_back(symbol->Name);
    }
    free(symbol);
#else
    void* array[100];
    int size = backtrace(array, 100);
    char** symbols = backtrace_symbols(array, size);

    if (symbols) {
        for (int i = 1; i < size; i++) {
            Dl_info info;
            if (dladdr(array[i], &info) && info.dli_sname) {
                int status;
                char* demangled = abi::__cxa_demangle(info.dli_sname, NULL, NULL, &status);
                trace.push_back(demangled ? demangled : info.dli_sname);
                free(demangled);
            }
            else {
                trace.push_back(symbols[i]);
            }
        }
        free(symbols);
    }
#endif

    return trace;
}