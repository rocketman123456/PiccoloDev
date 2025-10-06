#pragma once
#include "runtime/function/ecs/types.h"
#include "runtime/function/ecs/components.h"
#include "runtime/function/ecs/entt_coordinator.h"
#include "runtime/function/ecs/systems.h"
#include "runtime/function/ecs/ecs_serializer.h"
#include "runtime/function/ecs/postload_system.h"
#include "runtime/resource/res_type/common/level.h"
#include "runtime/function/framework/component/component.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

namespace Piccolo
{
    // 前向声明
    class GObject;
    
    // 关卡类 - 管理关卡级实体和游戏对象
    class Level
    {
    public:
        Level() = default;
        explicit Level(const std::string& name);
        ~Level() = default;
        
        // 禁用拷贝构造和赋值
        Level(const Level&) = delete;
        Level& operator=(const Level&) = delete;
        
        // 关卡管理
        void initialize();
        void shutdown();
        void update(float delta_time);
        
        // 关卡资源管理
        bool loadFromResource(const LevelRes& level_res);
        LevelRes saveToResource() const;
        
        // 实体管理
        Entity createEntity(const std::string& name = "");
        void destroyEntity(Entity entity);
        
        // 组件管理
        template<typename ComponentType, typename... Args>
        ComponentType& addComponent(Entity entity, Args&&... args)
        {
            return m_level_registry.emplace<ComponentType>(entity, std::forward<Args>(args)...);
        }
        
        template<typename ComponentType>
        void removeComponent(Entity entity)
        {
            m_level_registry.remove<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        ComponentType& getComponent(Entity entity)
        {
            return m_level_registry.get<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        const ComponentType& getComponent(Entity entity) const
        {
            return m_level_registry.get<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        bool hasComponent(Entity entity) const
        {
            return m_level_registry.all_of<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        ComponentType* tryGetComponent(Entity entity)
        {
            return m_level_registry.try_get<ComponentType>(entity);
        }
        
        template<typename ComponentType>
        const ComponentType* tryGetComponent(Entity entity) const
        {
            return m_level_registry.try_get<ComponentType>(entity);
        }
        
        // 实体查询
        template<typename... ComponentTypes>
        auto view()
        {
            return m_level_registry.view<ComponentTypes...>();
        }
        
        template<typename... ComponentTypes>
        auto view() const
        {
            return m_level_registry.view<ComponentTypes...>();
        }
        
        // GObject兼容性
        std::shared_ptr<GObject> createGObject(const std::string& name = "");
        void destroyGObject(std::shared_ptr<GObject> gobject);
        std::shared_ptr<GObject> getGObject(size_t object_id);
        
        // 实体和GObject映射
        Entity getEntityByObjectID(size_t object_id) const;
        size_t getObjectIDByEntity(Entity entity) const;
        
        // 获取关卡信息
        std::string getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }
        
        // 获取注册表
        Registry& getRegistry() { return m_level_registry; }
        const Registry& getRegistry() const { return m_level_registry; }
        
        // PostLoad支持
        void executePostLoadTasks();
        void addPostLoadTasks();
        
        // 系统管理
        void addSystem(std::unique_ptr<System> system);
        void removeSystem(const std::string& system_name);
        System* getSystem(const std::string& system_name);
        
        // 实体统计
        size_t getEntityCount() const { return 0; } // TODO: 实现正确的实体计数
        size_t getGObjectCount() const { return m_gobjects.size(); }
        
    private:
        std::string m_name;
        
        Registry m_level_registry;
        SystemManager m_system_manager;
        
        // GObject管理
        std::unordered_map<size_t, std::shared_ptr<GObject>> m_gobjects;
        size_t m_next_object_id = 1;
        
        // 实体和GObject映射
        std::unordered_map<size_t, Entity> m_object_id_to_entity;
        std::unordered_map<Entity, size_t> m_entity_to_object_id;
        
        // 实体名称映射
        std::unordered_map<std::string, Entity> m_entity_names;
        
        // 初始化默认系统
        void initializeDefaultSystems();
        
        // 创建关卡设置实体
        Entity createLevelSettingsEntity();
        
        // 生成新的对象ID
        size_t generateObjectID();
    };
    
    // 关卡管理器
    class LevelManager
    {
    public:
        static LevelManager& getInstance()
        {
            static LevelManager instance;
            return instance;
        }
        
        // 禁用拷贝构造和赋值
        LevelManager(const LevelManager&) = delete;
        LevelManager& operator=(const LevelManager&) = delete;
        
        // 关卡管理
        std::shared_ptr<Level> createLevel(const std::string& name);
        std::shared_ptr<Level> getCurrentLevel();
        void setCurrentLevel(const std::string& name);
        void destroyLevel(const std::string& name);
        
        // 关卡资源管理
        bool loadLevelFromResource(const std::string& name, const LevelRes& level_res);
        LevelRes saveLevelToResource(const std::string& name);
        
        // 更新当前关卡
        void update(float delta_time);
        
        // 获取所有关卡
        std::vector<std::string> getAllLevelNames() const;
        
        // 清空所有关卡
        void clearAllLevels();
        
    private:
        LevelManager() = default;
        ~LevelManager() = default;
        
        std::unordered_map<std::string, std::shared_ptr<Level>> m_levels;
        std::string m_current_level_name;
    };
    
    // 便捷的全局访问函数
    inline LevelManager& getLevelManager()
    {
        return LevelManager::getInstance();
    }
    
} // namespace Piccolo
