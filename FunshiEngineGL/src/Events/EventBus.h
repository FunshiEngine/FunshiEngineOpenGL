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
#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include <functional>
#include <unordered_map>

class GameObject;

enum class SceneEventType {
    ObjectCreated,
    ObjectDeleted,
    ObjectReparented,
    ComponentChanged,
    SceneCleared,
    ObjectSelected,
    ComponentStructureChanged
};

struct SceneEvent {
    SceneEventType type;
    GameObject* object = nullptr;
    GameObject* relatedObject = nullptr;
};

class EventBus {
private:
    using Callback = std::function<void(const SceneEvent&)>;
    std::unordered_map<size_t, Callback> listeners;
    size_t nextToken = 1;

public:
    size_t subscribe(Callback callback) ;

    void unsubscribe(size_t token) ;

    void publish(const SceneEvent& event) ;
};

#endif
