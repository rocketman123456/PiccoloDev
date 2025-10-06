#pragma once
#include "../../ecs/simple_ecs.h"
#include "runtime/resource/res_type/common/level.h"
#include "runtime/resource/res_type/common/object.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Piccolo
{
    // 简化的关卡类
    class SimpleLevel
    {
    public:
        SimpleLevel() = default;
        explicit SimpleLevel(const std::string& name);
        ~SimpleLevel() = default;
        
        // 禁用拷贝构造和赋值
        SimpleLevel(const SimpleLevel&) = delete;
        SimpleLevel& operator=(const SimpleLevel&) = delete;
        
        // 关卡管理
        void initialize();
        void shutdown();
        void update(float delta_time);
        
        // 关卡资源管理
        bool loadFromResource(const LevelRes& level_res);
        LevelRes saveToResource() const;
        
        // 实体管理
        Entity* createEntity(const std::string& name = "");
        void destroyEntity(EntityID entity_id);
        
        // 组件管理
        template<typename ComponentType, typename... Args>
        ComponentType* addComponent(EntityID entity_id, Args&&... args)
        {
            Entity* entity = m_entity_manager.getEntity(entity_id);
            if (entity)
            {
                return entity->addComponent<ComponentType>(std::forward<Args>(args)...);
            }
            return nullptr;
        }
        
        template<typename ComponentType>
        ComponentType* getComponent(EntityID entity_id)
        {
            Entity* entity = m_entity_manager.getEntity(entity_id);
            if (entity)
            {
                return entity->getComponent<ComponentType>();
            }
            return nullptr;
        }
        
        template<typename ComponentType>
        const ComponentType* getComponent(EntityID entity_id) const
        {
            const Entity* entity = m_entity_manager.getEntity(entity_id);
            if (entity)
            {
                return entity->getComponent<ComponentType>();
            }
            return nullptr;
        }
        
        template<typename ComponentType>
        bool hasComponent(EntityID entity_id) const
        {
            const Entity* entity = m_entity_manager.getEntity(entity_id);
            if (entity)
            {
                return entity->hasComponent<ComponentType>();
            }
            return false;
        }
        
        template<typename ComponentType>
        void removeComponent(EntityID entity_id)
        {
            Entity* entity = m_entity_manager.getEntity(entity_id);
            if (entity)
            {
                entity->removeComponent<ComponentType>();
            }
        }
        
        // 实体查询
        std::vector<Entity*> getEntitiesWithComponent(const std::string& component_type);
        std::vector<Entity*> getAllEntities();
        
        // 获取关卡信息
        std::string getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }
        
        // 获取实体管理器
        EntityManager& getEntityManager() { return m_entity_manager; }
        const EntityManager& getEntityManager() const { return m_entity_manager; }
        
        // PostLoad支持
        void executePostLoad();
        
        // 系统管理
        void addSystem(std::unique_ptr<System> system);
        
        // 实体统计
        size_t getEntityCount() const { return m_entity_manager.getEntityCount(); }
        
        // 从对象实例资源创建实体
        Entity* createEntityFromObjectInstance(const ObjectInstanceRes& object_instance);
        
        // 将实体序列化为对象实例资源
        ObjectInstanceRes serializeEntityToObjectInstance(EntityID entity_id) const;
        
    private:
        std::string m_name;
        
        EntityManager m_entity_manager;
        SystemManager m_system_manager;
        
        // 实体名称映射
        std::unordered_map<std::string, EntityID> m_entity_names;
        
        // 初始化默认系统
        void initializeDefaultSystems();
        
        // 创建关卡设置实体
        Entity* createLevelSettingsEntity();
    };
    
    // 简化的关卡管理器
    class SimpleLevelManager
    {
    public:
        static SimpleLevelManager& getInstance()
        {
            static SimpleLevelManager instance;
            return instance;
        }
        
        // 禁用拷贝构造和赋值
        SimpleLevelManager(const SimpleLevelManager&) = delete;
        SimpleLevelManager& operator=(const SimpleLevelManager&) = delete;
        
        // 关卡管理
        std::shared_ptr<SimpleLevel> createLevel(const std::string& name);
        std::shared_ptr<SimpleLevel> getCurrentLevel();
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
        SimpleLevelManager() = default;
        ~SimpleLevelManager() = default;
        
        std::unordered_map<std::string, std::shared_ptr<SimpleLevel>> m_levels;
        std::string m_current_level_name;
    };
    
    // 便捷的全局访问函数
    inline SimpleLevelManager& getSimpleLevelManager()
    {
        return SimpleLevelManager::getInstance();
    }
    
} // namespace Piccolo
