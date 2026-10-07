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
#ifndef SCRIPTAUDIOHANDLES_H
#define SCRIPTAUDIOHANDLES_H

#include <algorithm>
#include <vector>

class ScriptAudioHandles {
public:
    void registrar(int handle) {
        if (handle >= 0) handles_.push_back(handle);
    }

    void retirar(int handle) {
        handles_.erase(std::remove(handles_.begin(), handles_.end(), handle),
                       handles_.end());
    }

    template <typename Detener>
    void detenerTodos(Detener&& detener) {
        for (int handle : handles_) detener(handle);
        handles_.clear();
    }

private:
    std::vector<int> handles_;
};

#endif
