#pragma once
#include "../../ecs/simple_ecs.h"
#include "runtime/resource/res_type/common/world.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Piccolo
{
    // 简化的世界类
    class SimpleWorld
    {
    public:
        SimpleWorld() = default;
        explicit SimpleWorld(const std::string& name);
        ~SimpleWorld() = default;
        
        // 禁用拷贝构造和赋值
        SimpleWorld(const SimpleWorld&) = delete;
        SimpleWorld& operator=(const SimpleWorld&) = delete;
        
        // 世界管理
        void initialize();
        void shutdown();
        void update(float delta_time);
        
        // 世界资源管理
        bool loadFromResource(const WorldRes& world_res);
        WorldRes saveToResource() const;
        
        // 实体管理
        Entity* createWorldEntity(const std::string& name = "");
        void destroyWorldEntity(EntityID entity_id);
        
        // 关卡管理
        void addLevel(const std::string& level_url);
        void removeLevel(const std::string& level_url);
        std::vector<std::string> getLevelUrls() const;
        std::string getDefaultLevelUrl() const;
        void setDefaultLevelUrl(const std::string& level_url);
        
        // 获取世界信息
        std::string getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }
        
        // 获取实体管理器
        EntityManager& getEntityManager() { return m_entity_manager; }
        const EntityManager& getEntityManager() const { return m_entity_manager; }
        
        // PostLoad支持
        void executePostLoad();
        
        // 系统管理
        void addSystem(std::unique_ptr<System> system);
        
    private:
        std::string m_name;
        std::string m_default_level_url;
        std::vector<std::string> m_level_urls;
        
        EntityManager m_entity_manager;
        SystemManager m_system_manager;
        
        // 世界级实体映射
        std::unordered_map<std::string, EntityID> m_world_entities;
        
        // 初始化默认系统
        void initializeDefaultSystems();
        
        // 创建世界级实体
        Entity* createWorldSettingsEntity();
    };
    
    // 简化的世界管理器
    class SimpleWorldManager
    {
    public:
        static SimpleWorldManager& getInstance()
        {
            static SimpleWorldManager instance;
            return instance;
        }
        
        // 禁用拷贝构造和赋值
        SimpleWorldManager(const SimpleWorldManager&) = delete;
        SimpleWorldManager& operator=(const SimpleWorldManager&) = delete;
        
        // 世界管理
        std::shared_ptr<SimpleWorld> createWorld(const std::string& name);
        std::shared_ptr<SimpleWorld> getCurrentWorld();
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
        SimpleWorldManager() = default;
        ~SimpleWorldManager() = default;
        
        std::unordered_map<std::string, std::shared_ptr<SimpleWorld>> m_worlds;
        std::string m_current_world_name;
    };
    
    // 便捷的全局访问函数
    inline SimpleWorldManager& getSimpleWorldManager()
    {
        return SimpleWorldManager::getInstance();
    }
    
} // namespace Piccolo
