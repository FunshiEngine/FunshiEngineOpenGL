#ifndef ENTITY_H
#define ENTITY_H

#include <memory>
#include <string>
#include <vector>

#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Objetos/Componentes/Component.h"
#include "../GestorDeArchivos/Binario.h"
#include "../Objetos/Componentes/Transform.h"

// Declaracion adelantada: al leer los componentes hace falta el dueno (el
// componente se crea con el y su hook de post-carga lo recibe).
class GameObject;

// Base de toda entidad de la escena: es duena de sus componentes, conoce su
// lugar en la jerarquia y sabe como escribir y leer su estado en el binario.
//
// La parte comun de la serializacion (el orden de los bloques, el transform
// de origen y la lista de componentes) vive aqui; el derivado solo aporta los
// atributos que son suyos (id, nombre, estado, tag), cuyo formato es parte del
// binario de escena.
class Entity {
protected:
    // The list is a non-owning compatibility view. componentOwners owns every
    // component stored in it.
    std::unique_ptr<ListaDE<Component*>> components;
    std::vector<std::unique_ptr<Component>> componentOwners;
    std::unique_ptr<Binario> myBinario;
    Entity* parentEntity = nullptr;
    std::vector<Entity*> childEntities;
    Transform* transformOrigin = nullptr;
    std::unique_ptr<Transform> ownedTransformOrigin;
    // Buffer reutilizable para el transform global. NO puede ser un
    // unique_ptr recreado por llamada: getGlobalTransform() devuelve puntero a
    // este miembro y otros sistemas (gizmo, colliders, settings) lo guardan
    // entre frames; si se reasignara, quedarian punteros a memoria liberada
    // (heap-use-after-free). El valor se sobrescribe en cada llamada, pero la
    // direccion es estable mientras la entidad viva.
    Transform globalTransformCache;

    void clearComponents();

    // Atributos que son de la entidad concreta (id, nombre, estado, tag).
    virtual void serializeLocalAtributes() = 0;
    virtual void deserializeLocalAtributes() = 0;

    // Bloques del binario que no dependen del tipo de entidad.
    void serializeGlobalAtributes();
    void serializeExternalAtributes();
    void serializeTransformOrigin();
    void deserializeGlobalAtributes();
    void deserializeExternalAtributes();
    void deserializeTransformOrigin();
    void serializeEntityComponents();
    // Los componentes se crean con su dueno (y su hook de post-carga lo
    // recibe), asi que quien los lee pasa el objeto que los contiene.
    void deserializeEntityComponents(GameObject& owner);
    // Punto de extension: una entidad concreta (p. ej. el modelo, que guarda
    // la ruta del archivo) escribe despues su parte propia.
    virtual void serializeEntity();
    virtual void deserializeEntity();

public:
    Entity();
    Entity(Entity* origin);
    virtual ~Entity();

    Binario* getMyBinario();
    void setOriginTransform(Transform* newOriginTransform);
    Transform* getOriginTransform() const { return transformOrigin; }

    void setParentEntity(Entity* parent);
    Entity* getParentEntity() const { return parentEntity; }
    const std::vector<Entity*>& getChildEntities() const { return childEntities; }

    // Componentes: la entidad es duena de los suyos.
    void addComponent(Component* component);
    void addComponent(std::unique_ptr<Component> component);
    void deleteComponent(Component* component);
    std::unique_ptr<Component> extractComponent(Component* component);
    Component* getComponentByName(const std::string& typeName);
    bool hasComponent(const std::string& typeName);
    ListaDE<Component*>* getComponents();

    template <typename T>
    T* getComponent() {
        if (!components || components->isEmpty()) return nullptr;
        Position<Component*>* position = components->first();
        while (position != nullptr) {
            if (auto* component = dynamic_cast<T*>(position->getElement())) return component;
            position = (position != components->last()) ? components->next(position) : nullptr;
        }
        return nullptr;
    }

    // Version de solo lectura: deja consultar los componentes de un objeto
    // constante (por ejemplo, para resolver el material de una region sin
    // poder modificar el objeto).
    template <typename T>
    const T* getComponent() const {
        if (!components || components->isEmpty()) return nullptr;
        Position<Component*>* position = components->first();
        while (position != nullptr) {
            if (auto* component = dynamic_cast<const T*>(position->getElement()))
                return component;
            position = (position != components->last()) ? components->next(position) : nullptr;
        }
        return nullptr;
    }

    // Devuelven false si el archivo binario no se pudo abrir (sin directorio,
    // sin permisos): quien llama (SceneSerializer) lo avisa en el log en vez de
    // dejar que cada write()/read() falle en silencio. El nombre del archivo
    // lo decide el derivado (cada entidad se guarda en su propio archivo).
    virtual bool saveEntity(std::string filename) = 0;
    virtual bool loadEntity(std::string filename) = 0;

    // Transform global (el local compuesto con el de los padres).
    virtual Transform* getGlobalTransform();
};

#endif
