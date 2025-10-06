#include "simple_world.h"

namespace Piccolo
{
    // SimpleWorld 实现
    SimpleWorld::SimpleWorld(const std::string& name) : m_name(name)
    {
        initialize();
    }
    
    void SimpleWorld::initialize()
    {
        // 初始化默认系统
        initializeDefaultSystems();
        
        // 创建世界设置实体
        createWorldSettingsEntity();
    }
    
    void SimpleWorld::shutdown()
    {
        // 关闭所有系统
        m_system_manager.shutdown(m_entity_manager);
        
        // 清空实体管理器
        // EntityManager会在析构时自动清理
    }
    
    void SimpleWorld::update(float delta_time)
    {
        // 更新所有系统
        m_system_manager.update(m_entity_manager, delta_time);
    }
    
    bool SimpleWorld::loadFromResource(const WorldRes& world_res)
    {
        m_name = world_res.m_name;
        m_level_urls = world_res.m_level_urls;
        m_default_level_url = world_res.m_default_level_url;
        
        // 创建世界级实体
        Entity* world_entity = createWorldEntity("WorldSettings");
        if (world_entity)
        {
            // 这里可以添加世界级组件
        }
        
        // 执行PostLoad
        executePostLoad();
        
        return true;
    }
    
    WorldRes SimpleWorld::saveToResource() const
    {
        WorldRes world_res;
        world_res.m_name = m_name;
        world_res.m_level_urls = m_level_urls;
        world_res.m_default_level_url = m_default_level_url;
        
        return world_res;
    }
    
    Entity* SimpleWorld::createWorldEntity(const std::string& name)
    {
        Entity* entity = m_entity_manager.createEntity();
        
        if (!name.empty())
        {
            entity->addComponent<NameComponent>(name);
            m_world_entities[name] = entity->getID();
        }
        
        return entity;
    }
    
    void SimpleWorld::destroyWorldEntity(EntityID entity_id)
    {
        // 从映射中移除
        for (auto it = m_world_entities.begin(); it != m_world_entities.end(); ++it)
        {
            if (it->second == entity_id)
            {
                m_world_entities.erase(it);
                break;
            }
        }
        
        m_entity_manager.destroyEntity(entity_id);
    }
    
    void SimpleWorld::addLevel(const std::string& level_url)
    {
        auto it = std::find(m_level_urls.begin(), m_level_urls.end(), level_url);
        if (it == m_level_urls.end())
        {
            m_level_urls.push_back(level_url);
        }
    }
    
    void SimpleWorld::removeLevel(const std::string& level_url)
    {
        auto it = std::find(m_level_urls.begin(), m_level_urls.end(), level_url);
        if (it != m_level_urls.end())
        {
            m_level_urls.erase(it);
        }
    }
    
    std::vector<std::string> SimpleWorld::getLevelUrls() const
    {
        return m_level_urls;
    }
    
    std::string SimpleWorld::getDefaultLevelUrl() const
    {
        return m_default_level_url;
    }
    
    void SimpleWorld::setDefaultLevelUrl(const std::string& level_url)
    {
        m_default_level_url = level_url;
    }
    
    void SimpleWorld::executePostLoad()
    {
        m_entity_manager.postLoad();
    }
    
    void SimpleWorld::addSystem(std::unique_ptr<System> system)
    {
        m_system_manager.addSystem(std::move(system));
    }
    
    void SimpleWorld::initializeDefaultSystems()
    {
        // 添加默认系统
        m_system_manager.addSystem(std::make_unique<TransformSystem>());
        
        // 初始化系统
        m_system_manager.initialize(m_entity_manager);
    }
    
    Entity* SimpleWorld::createWorldSettingsEntity()
    {
        Entity* world_entity = createWorldEntity("WorldSettings");
        return world_entity;
    }
    
    // SimpleWorldManager 实现
    std::shared_ptr<SimpleWorld> SimpleWorldManager::createWorld(const std::string& name)
    {
        auto world = std::make_shared<SimpleWorld>(name);
        m_worlds[name] = world;
        
        if (m_current_world_name.empty())
        {
            m_current_world_name = name;
        }
        
        return world;
    }
    
    std::shared_ptr<SimpleWorld> SimpleWorldManager::getCurrentWorld()
    {
        if (m_current_world_name.empty() || m_worlds.find(m_current_world_name) == m_worlds.end())
        {
            return nullptr;
        }
        
        return m_worlds[m_current_world_name];
    }
    
    void SimpleWorldManager::setCurrentWorld(const std::string& name)
    {
        if (m_worlds.find(name) != m_worlds.end())
        {
            m_current_world_name = name;
        }
    }
    
    void SimpleWorldManager::destroyWorld(const std::string& name)
    {
        auto it = m_worlds.find(name);
        if (it != m_worlds.end())
        {
            it->second->shutdown();
            m_worlds.erase(it);
            
            if (m_current_world_name == name)
            {
                m_current_world_name.clear();
            }
        }
    }
    
    bool SimpleWorldManager::loadWorldFromResource(const std::string& name, const WorldRes& world_res)
    {
        auto world = createWorld(name);
        return world->loadFromResource(world_res);
    }
    
    WorldRes SimpleWorldManager::saveWorldToResource(const std::string& name)
    {
        auto it = m_worlds.find(name);
        if (it != m_worlds.end())
        {
            return it->second->saveToResource();
        }
        
        return WorldRes();
    }
    
    void SimpleWorldManager::update(float delta_time)
    {
        if (auto current_world = getCurrentWorld())
        {
            current_world->update(delta_time);
        }
    }
    
    std::vector<std::string> SimpleWorldManager::getAllWorldNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : m_worlds)
        {
            names.push_back(pair.first);
        }
        return names;
    }
    
    void SimpleWorldManager::clearAllWorlds()
    {
        for (auto& pair : m_worlds)
        {
            pair.second->shutdown();
        }
        m_worlds.clear();
        m_current_world_name.clear();
    }
    
} // namespace Piccolo
