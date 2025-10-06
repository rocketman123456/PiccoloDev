#pragma once
#include "runtime/function/ecs/types.h"
#include "runtime/function/ecs/components.h"
#include "runtime/function/ecs/entt_coordinator.h"
#include "runtime/function/ecs/systems.h"
#include "runtime/function/ecs/ecs_serializer.h"
#include "runtime/function/ecs/postload_system.h"
#include "runtime/resource/res_type/common/world.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Piccolo
{
    // 世界类 - 管理世界级实体和关卡
    class World
    {
    public:
        World() = default;
        explicit World(const std::string& name);
        ~World() = default;
        
        // 禁用拷贝构造和赋值
        World(const World&) = delete;
        World& operator=(const World&) = delete;
        
        // 世界管理
        void initialize();
        void shutdown();
        void update(float delta_time);
        
        // 世界资源管理
        bool loadFromResource(const WorldRes& world_res);
        WorldRes saveToResource() const;
        
        // 实体管理
        Entity createWorldEntity(const std::string& name = "");
        void destroyWorldEntity(Entity entity);
        
        // 组件管理
        template<typename ComponentType, typename... Args>
        ComponentType& addWorldComponent(Entity entity, Args&&... args)
        {
            return m_world_registry.emplace<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        template<typename ComponentType>
        void removeWorldComponent(Entity entity)
        {
            m_world_registry.remove<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        ComponentType& getWorldComponent(Entity entity)
        {
            return m_world_registry.get<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        const ComponentType& getWorldComponent(Entity entity) const
        {
            return m_world_registry.get<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        bool hasWorldComponent(Entity entity) const
        {
            return m_world_registry.all_of<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        ComponentType* tryGetWorldComponent(Entity entity)
        {
            return m_world_registry.try_get<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        const ComponentType* tryGetWorldComponent(Entity entity) const
        {
            return m_world_registry.try_get<ComponentType>(entity);
        }
        
        // 实体查询
        template<typename... ComponentTypes>
        auto viewWorld()
        {
            return m_world_registry.view<ComponentTypes...>();
        }
        
        template<typename... ComponentTypes>
        auto viewWorld() const
        {
            return m_world_registry.view<ComponentTypes...>();
        }
        
        // 关卡管理
        void addLevel(const std::string& level_url);
        void removeLevel(const std::string& level_url);
        std::vector<std::string> getLevelUrls() const;
        std::string getDefaultLevelUrl() const;
        void setDefaultLevelUrl(const std::string& level_url);
        
        // 获取世界信息
        std::string getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }
        
        // 获取注册表
        Registry& getWorldRegistry() { return m_world_registry; }
        const Registry& getWorldRegistry() const { return m_world_registry; }
        
        // PostLoad支持
        void executePostLoadTasks();
        void addPostLoadTasks();
        
        // 系统管理
        void addSystem(std::unique_ptr<System> system);
        void removeSystem(const std::string& system_name);
        System* getSystem(const std::string& system_name);
        
    private:
        std::string m_name;
        std::string m_default_level_url;
        std::vector<std::string> m_level_urls;
        
        Registry m_world_registry;
        SystemManager m_system_manager;
        
        // 世界级实体映射
        std::unordered_map<std::string, Entity> m_world_entities;
        
        // 初始化默认系统
        void initializeDefaultSystems();
        
        // 创建世界级实体
        Entity createWorldSettingsEntity();
    };
    
    // 世界管理器
    class WorldManager
    {
    public:
        static WorldManager& getInstance()
        {
            static WorldManager instance;
            return instance;
        }
        
        // 禁用拷贝构造和赋值
        WorldManager(const WorldManager&) = delete;
        WorldManager& operator=(const WorldManager&) = delete;
        
        // 世界管理
        std::shared_ptr<World> createWorld(const std::string& name);
        std::shared_ptr<World> getCurrentWorld();
        void setCurrentWorld(const std::string& name);
        void destroyWorld(const std::string& name);
        
        // 世界资源管理
        bool loadWorldFromResource(const std::string& name, const WorldRes& world_res);
        WorldRes saveWorldToResource(const std::string& name);
        
        // 更新当前世界
        void update(float delta_time);
        
        // 获取所有世界
        std::vector<std::string> getAllWorldNames() const;
        
        // 清空所有世界
        void clearAllWorlds();
        
    private:
        WorldManager() = default;
        ~WorldManager() = default;
        
        std::unordered_map<std::string, std::shared_ptr<World>> m_worlds;
        std::string m_current_world_name;
    };
    
    // 便捷的全局访问函数
    inline WorldManager& getWorldManager()
    {
        return WorldManager::getInstance();
    }
    
} // namespace Piccolo
