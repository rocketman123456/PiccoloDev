#pragma once
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

namespace Piccolo
{
    // 简化的ECS系统，不依赖entt库
    using EntityID = uint32_t;
    constexpr EntityID INVALID_ENTITY = 0;
    
    // 基础组件接口
    class Component
    {
    public:
        virtual ~Component() = default;
        virtual void update(float delta_time) {}
        virtual void postLoad() {}
    };
    
    // 实体类
    class Entity
    {
    public:
        explicit Entity(EntityID id) : m_id(id) {}
        
        EntityID getID() const { return m_id; }
        
        template<typename T, typename... Args>
        T* addComponent(Args&&... args)
        {
            auto component = std::make_unique<T>(std::forward<Args>(args)...);
            T* ptr = component.get();
            m_components[typeid(T).name()] = std::move(component);
            return ptr;
        }
        
        template<typename T>
        T* getComponent()
        {
            auto it = m_components.find(typeid(T).name());
            if (it != m_components.end())
            {
                return static_cast<T*>(it->second.get());
            }
            return nullptr;
        }
        
        template<typename T>
        const T* getComponent() const
        {
            auto it = m_components.find(typeid(T).name());
            if (it != m_components.end())
            {
                return static_cast<const T*>(it->second.get());
            }
            return nullptr;
        }
        
        template<typename T>
        bool hasComponent() const
        {
            return m_components.find(typeid(T).name()) != m_components.end();
        }
        
        template<typename T>
        void removeComponent()
        {
            m_components.erase(typeid(T).name());
        }
        
        void update(float delta_time)
        {
            for (auto& pair : m_components)
            {
                pair.second->update(delta_time);
            }
        }
        
        void postLoad()
        {
            for (auto& pair : m_components)
            {
                pair.second->postLoad();
            }
        }
        
    private:
        EntityID m_id;
        std::unordered_map<std::string, std::unique_ptr<Component>> m_components;
    };
    
    // 实体管理器
    class EntityManager
    {
    public:
        EntityManager() : m_next_id(1) {}
        
        Entity* createEntity()
        {
            EntityID id = m_next_id++;
            auto entity = std::make_unique<Entity>(id);
            Entity* ptr = entity.get();
            m_entities[id] = std::move(entity);
            return ptr;
        }
        
        void destroyEntity(EntityID id)
        {
            m_entities.erase(id);
        }
        
        Entity* getEntity(EntityID id)
        {
            auto it = m_entities.find(id);
            if (it != m_entities.end())
            {
                return it->second.get();
            }
            return nullptr;
        }
        
        Entity* getEntity(EntityID id) const
        {
            auto it = m_entities.find(id);
            if (it != m_entities.end())
            {
                return it->second.get();
            }
            return nullptr;
        }
        
        std::vector<Entity*> getAllEntities()
        {
            std::vector<Entity*> result;
            result.reserve(m_entities.size());
            for (auto& pair : m_entities)
            {
                result.push_back(pair.second.get());
            }
            return result;
        }
        
        std::vector<const Entity*> getAllEntities() const
        {
            std::vector<const Entity*> result;
            result.reserve(m_entities.size());
            for (const auto& pair : m_entities)
            {
                result.push_back(pair.second.get());
            }
            return result;
        }
        
        void update(float delta_time)
        {
            for (auto& pair : m_entities)
            {
                pair.second->update(delta_time);
            }
        }
        
        void postLoad()
        {
            for (auto& pair : m_entities)
            {
                pair.second->postLoad();
            }
        }
        
        size_t getEntityCount() const { return m_entities.size(); }
        
    private:
        EntityID m_next_id;
        std::unordered_map<EntityID, std::unique_ptr<Entity>> m_entities;
    };
    
    // 基础组件类型
    struct TransformComponent : public Component
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float rx = 0.0f, ry = 0.0f, rz = 0.0f;
        float sx = 1.0f, sy = 1.0f, sz = 1.0f;
        
        TransformComponent() = default;
        explicit TransformComponent(float x, float y, float z) : x(x), y(y), z(z) {}
    };
    
    struct NameComponent : public Component
    {
        std::string name;
        bool active = true;
        
        NameComponent() = default;
        NameComponent(const std::string& n, bool a = true) : name(n), active(a) {}
    };
    
    struct RenderableComponent : public Component
    {
        std::string mesh_url;
        std::string material_url;
        bool visible = true;
        
        RenderableComponent() = default;
        RenderableComponent(const std::string& mesh, const std::string& material)
            : mesh_url(mesh), material_url(material) {}
    };
    
    struct CameraComponent : public Component
    {
        float fov = 45.0f;
        float near_plane = 0.1f;
        float far_plane = 1000.0f;
        bool is_main_camera = false;
        
        CameraComponent() = default;
        CameraComponent(float f, float near_p, float far_p, bool main = false)
            : fov(f), near_plane(near_p), far_plane(far_p), is_main_camera(main) {}
    };
    
    struct LightComponent : public Component
    {
        enum class LightType
        {
            Directional,
            Point,
            Spot
        };
        
        LightType type = LightType::Directional;
        float r = 1.0f, g = 1.0f, b = 1.0f;
        float intensity = 1.0f;
        float range = 10.0f;
        bool cast_shadow = true;
        
        LightComponent() = default;
        LightComponent(LightType t, float r, float g, float b, float i)
            : type(t), r(r), g(g), b(b), intensity(i) {}
    };
    
    // 系统基类
    class System
    {
    public:
        virtual ~System() = default;
        virtual void update(EntityManager& entity_manager, float delta_time) = 0;
        virtual void initialize(EntityManager& entity_manager) {}
        virtual void shutdown(EntityManager& entity_manager) {}
    };
    
    // 变换系统
    class TransformSystem : public System
    {
    public:
        void update(EntityManager& entity_manager, float /*delta_time*/) override
        {
            auto entities = entity_manager.getAllEntities();
            for (auto* entity : entities)
            {
                if (entity->hasComponent<TransformComponent>())
                {
                    auto* transform = entity->getComponent<TransformComponent>();
                    // 处理变换更新逻辑
                    (void)transform; // 避免未使用变量警告
                }
            }
        }
    };
    
    // 渲染系统
    class RenderSystem : public System
    {
    public:
        void update(EntityManager& entity_manager, float /*delta_time*/) override
        {
            auto entities = entity_manager.getAllEntities();
            for (auto* entity : entities)
            {
                if (entity->hasComponent<TransformComponent>() && 
                    entity->hasComponent<RenderableComponent>())
                {
                    auto* transform = entity->getComponent<TransformComponent>();
                    auto* renderable = entity->getComponent<RenderableComponent>();
                    
                    if (renderable->visible)
                    {
                        // 处理渲染逻辑
                        (void)transform; // 避免未使用变量警告
                    }
                }
            }
        }
    };
    
    // 系统管理器
    class SystemManager
    {
    public:
        void addSystem(std::unique_ptr<System> system)
        {
            m_systems.push_back(std::move(system));
        }
        
        void update(EntityManager& entity_manager, float delta_time)
        {
            for (auto& system : m_systems)
            {
                system->update(entity_manager, delta_time);
            }
        }
        
        void initialize(EntityManager& entity_manager)
        {
            for (auto& system : m_systems)
            {
                system->initialize(entity_manager);
            }
        }
        
        void shutdown(EntityManager& entity_manager)
        {
            for (auto& system : m_systems)
            {
                system->shutdown(entity_manager);
            }
        }
        
    private:
        std::vector<std::unique_ptr<System>> m_systems;
    };
    
} // namespace Piccolo
