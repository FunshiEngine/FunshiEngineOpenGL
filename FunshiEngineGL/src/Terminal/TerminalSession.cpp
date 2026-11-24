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
#include "TerminalSessionDetalle.h"

TerminalSession::TerminalSession() : impl_(std::make_unique<Impl>()) {}

TerminalSession::~TerminalSession() { detener(); }

bool TerminalSession::iniciar(const std::string& shell,
                              const std::string& directorio,
                              unsigned columnas, unsigned filas,
                              std::string& error) {
    detener();
    if (shell.empty() || directorio.empty()) {
        error = "La shell y el directorio de trabajo son obligatorios";
        return false;
    }
    std::error_code errorRuta;
    if (!std::filesystem::is_directory(std::filesystem::u8path(directorio), errorRuta)) {
        error = "El directorio del proyecto no está disponible";
        return false;
    }
    columnas = std::max(1u, columnas);
    filas = std::max(1u, filas);

#if defined(_WIN32)
    SECURITY_ATTRIBUTES seguridad{};
    seguridad.nLength = sizeof(seguridad);
    seguridad.bInheritHandle = FALSE;
    HANDLE entradaLectura = INVALID_HANDLE_VALUE;
    HANDLE salidaEscritura = INVALID_HANDLE_VALUE;
    HANDLE entradaEscritura = INVALID_HANDLE_VALUE;
    HANDLE salidaLectura = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&entradaLectura, &entradaEscritura, &seguridad, 0) ||
        !CreatePipe(&salidaLectura, &salidaEscritura, &seguridad, 0)) {
        error = "No se pudieron crear los canales de ConPTY";
        goto fallo_windows;
    }
    SetHandleInformation(entradaEscritura, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(salidaLectura, HANDLE_FLAG_INHERIT, 0);

    {
        const COORD tamano{static_cast<SHORT>(std::min(columnas, 32767u)),
                           static_cast<SHORT>(std::min(filas, 32767u))};
        const HRESULT creado = CreatePseudoConsole(
            tamano, entradaLectura, salidaEscritura, 0,
            &impl_->pseudoconsola);
        if (FAILED(creado)) {
            error = "ConPTY no está disponible en este Windows";
            goto fallo_windows;
        }
    }
    {
        SIZE_T bytesAtributos = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &bytesAtributos);
        std::vector<unsigned char> almacenamiento(bytesAtributos);
        auto* atributos = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            almacenamiento.data());
        if (!InitializeProcThreadAttributeList(atributos, 1, 0,
                                              &bytesAtributos)) {
            error = "No se pudo preparar el inicio de la shell ConPTY";
            goto fallo_windows;
        }
        if (!UpdateProcThreadAttribute(
                atributos, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                impl_->pseudoconsola, sizeof(impl_->pseudoconsola), nullptr,
                nullptr)) {
            DeleteProcThreadAttributeList(atributos);
            error = "No se pudo asociar la shell a ConPTY";
            goto fallo_windows;
        }

        std::wstring ejecutable = convertirAncho(shell);
        if (ejecutable.find_first_of(L"/\\") == std::wstring::npos) {
            std::vector<wchar_t> rutaEjecutable(32768, L'\0');
            const DWORD longitud = SearchPathW(nullptr, ejecutable.c_str(), nullptr,
                                               static_cast<DWORD>(rutaEjecutable.size()),
                                               rutaEjecutable.data(), nullptr);
            if (longitud == 0 || longitud >= rutaEjecutable.size()) {
                error = "No se encontro el ejecutable de la shell";
                DeleteProcThreadAttributeList(atributos);
                goto fallo_windows;
            }
            ejecutable.assign(rutaEjecutable.data(), longitud);
        }
        std::wstring directorioAncho = convertirAncho(directorio);
        std::wstring linea = L"\"" + ejecutable + L"\"";
        if (shell == "cmd.exe") linea += L" /Q";
        else if (shell == "powershell.exe") linea += L" -NoLogo -NoExit";
        else if (shell == "pwsh.exe") linea += L" -NoLogo -NoExit";
        STARTUPINFOEXW inicio{};
        inicio.StartupInfo.cb = sizeof(inicio);
        inicio.lpAttributeList = atributos;
        PROCESS_INFORMATION info{};
        const BOOL iniciado = CreateProcessW(
            nullptr, &linea[0], nullptr, nullptr, FALSE,
            EXTENDED_STARTUPINFO_PRESENT, nullptr, directorioAncho.c_str(),
            &inicio.StartupInfo, &info);
        DeleteProcThreadAttributeList(atributos);
        if (!iniciado) {
            error = "No se pudo iniciar la shell solicitada";
            goto fallo_windows;
        }
        CloseHandle(entradaLectura);
        entradaLectura = INVALID_HANDLE_VALUE;
        CloseHandle(salidaEscritura);
        salidaEscritura = INVALID_HANDLE_VALUE;
        CloseHandle(info.hThread);
        impl_->trabajo = CreateJobObjectW(nullptr, nullptr);
        if (!impl_->trabajo) {
            error = "No se pudo crear el grupo de procesos de la shell";
            TerminateProcess(info.hProcess, 1);
            CloseHandle(info.hProcess);
            goto fallo_windows;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limites{};
        limites.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(impl_->trabajo,
                                     JobObjectExtendedLimitInformation,
                                     &limites, sizeof(limites)) ||
            !AssignProcessToJobObject(impl_->trabajo, info.hProcess)) {
            error = "No se pudo asociar la shell a su grupo de procesos";
            TerminateProcess(info.hProcess, 1);
            CloseHandle(info.hProcess);
            CloseHandle(impl_->trabajo);
            impl_->trabajo = nullptr;
            goto fallo_windows;
        }
        impl_->proceso = info.hProcess;
    }
    impl_->entrada = entradaEscritura;
    impl_->salidaPipe = salidaLectura;
    impl_->activa.store(true, std::memory_order_release);
    impl_->hiloLectura = std::thread([this] { impl_->leer(); });
    return true;

fallo_windows:
    if (entradaLectura != INVALID_HANDLE_VALUE) CloseHandle(entradaLectura);
    if (entradaEscritura != INVALID_HANDLE_VALUE) CloseHandle(entradaEscritura);
    if (salidaLectura != INVALID_HANDLE_VALUE) CloseHandle(salidaLectura);
    if (salidaEscritura != INVALID_HANDLE_VALUE) CloseHandle(salidaEscritura);
    if (impl_->pseudoconsola) {
        ClosePseudoConsole(impl_->pseudoconsola);
        impl_->pseudoconsola = nullptr;
    }
    return false;
#else
    if (::access(shell.c_str(), X_OK) != 0) {
        error = "La shell seleccionada no está instalada: " + shell;
        return false;
    }
    struct winsize tamano{};
    tamano.ws_col = static_cast<unsigned short>(
        std::min(columnas, static_cast<unsigned>(USHRT_MAX)));
    tamano.ws_row = static_cast<unsigned short>(
        std::min(filas, static_cast<unsigned>(USHRT_MAX)));
    const pid_t hijo = forkpty(&impl_->terminal, nullptr, nullptr, &tamano);
    if (hijo < 0) {
        error = "No se pudo crear el pseudo-terminal: " +
                std::string(std::strerror(errno));
        return false;
    }
    if (hijo == 0) {
        if (::chdir(directorio.c_str()) != 0) _exit(126);
        char* argumentos[] = {const_cast<char*>(shell.c_str()),
                              const_cast<char*>("-i"), nullptr};
        ::execvp(argumentos[0], argumentos);
        static constexpr char mensaje[] =
            "No se pudo iniciar la shell seleccionada\n";
        ::write(STDERR_FILENO, mensaje, sizeof(mensaje) - 1);
        _exit(127);
    }
    impl_->proceso = hijo;
    const int banderas = ::fcntl(impl_->terminal, F_GETFL, 0);
    if (banderas >= 0) ::fcntl(impl_->terminal, F_SETFL, banderas | O_NONBLOCK);
    impl_->parada.store(false, std::memory_order_release);
#if !defined(_WIN32)
    impl_->lecturaTerminada.store(false, std::memory_order_release);
#endif
    impl_->hijoReapeado.store(false, std::memory_order_release);
    impl_->activa.store(true, std::memory_order_release);
    impl_->hiloLectura = std::thread([this] { impl_->leer(); });
    return true;
#endif
}
