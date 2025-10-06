#include "world.h"
#include "runtime/function/global/global_context.h"

#include <algorithm>

namespace Piccolo
{
    // World 实现
    World::World(const std::string& name) : m_name(name)
    {
        initialize();
    }
    
    void World::initialize()
    {
        // 初始化默认系统
        initializeDefaultSystems();
        
        // 创建世界设置实体
        createWorldSettingsEntity();
    }
    
    void World::shutdown()
    {
        // 关闭所有系统
        m_system_manager.shutdown(m_world_registry);
        
        // 清空注册表
        m_world_registry.clear();
        
        // 清空映射
        m_world_entities.clear();
    }
    
    void World::update(float delta_time)
    {
        // 更新所有系统
        m_system_manager.update(m_world_registry, delta_time);
    }
    
    bool World::loadFromResource(const WorldRes& world_res)
    {
        m_name = world_res.m_name;
        m_level_urls = world_res.m_level_urls;
        m_default_level_url = world_res.m_default_level_url;
        
        // 使用转换器从资源创建实体
        auto& converter = getResourceToECSConverter();
        auto entities = converter.convertWorldToEntities(world_res, m_world_registry);
        
        // 添加PostLoad任务
        addPostLoadTasks();
        
        return true;
    }
    
    WorldRes World::saveToResource() const
    {
        WorldRes world_res;
        world_res.m_name = m_name;
        world_res.m_level_urls = m_level_urls;
        world_res.m_default_level_url = m_default_level_url;
        
        return world_res;
    }
    
    Entity World::createWorldEntity(const std::string& name)
    {
        Entity entity = m_world_registry.create();
        
        if (!name.empty())
        {
            addWorldComponent<NameComponent>(entity, name);
            m_world_entities[name] = entity;
        }
        
        return entity;
    }
    
    void World::destroyWorldEntity(Entity entity)
    {
        // 从映射中移除
        if (m_world_registry.all_of<NameComponent>(entity))
        {
            const auto& name_comp = m_world_registry.get<NameComponent>(entity);
            auto it = m_world_entities.find(name_comp.name);
            if (it != m_world_entities.end())
            {
                m_world_entities.erase(it);
            }
        }
        
        m_world_registry.destroy(entity);
    }
    
    void World::addLevel(const std::string& level_url)
    {
        auto it = std::find(m_level_urls.begin(), m_level_urls.end(), level_url);
        if (it == m_level_urls.end())
        {
            m_level_urls.push_back(level_url);
        }
    }
    
    void World::removeLevel(const std::string& level_url)
    {
        auto it = std::find(m_level_urls.begin(), m_level_urls.end(), level_url);
        if (it != m_level_urls.end())
        {
            m_level_urls.erase(it);
        }
    }
    
    std::vector<std::string> World::getLevelUrls() const
    {
        return m_level_urls;
    }
    
    std::string World::getDefaultLevelUrl() const
    {
        return m_default_level_url;
    }
    
    void World::setDefaultLevelUrl(const std::string& level_url)
    {
        m_default_level_url = level_url;
    }
    
    void World::executePostLoadTasks()
    {
        auto& postload_manager = getPostLoadManager();
        postload_manager.executeAllPostLoadTasks();
    }
    
    void World::addPostLoadTasks()
    {
        auto& postload_manager = getPostLoadManager();
        postload_manager.addPostLoadTasksForRegistry(m_world_registry);
    }
    
    void World::addSystem(std::unique_ptr<System> system)
    {
        m_system_manager.addSystem(std::move(system));
    }
    
    void World::removeSystem(const std::string& system_name)
    {
        // 这里需要根据系统名称移除系统，但System基类没有名称
        // 可以考虑为System添加名称成员或者使用typeid
    }
    
    System* World::getSystem(const std::string& system_name)
    {
        // 这里需要根据系统名称获取系统，但System基类没有名称
        // 可以考虑为System添加名称成员或者使用typeid
        return nullptr;
    }
    
    void World::initializeDefaultSystems()
    {
        // 添加默认系统
        m_system_manager.addSystem<TransformSystem>();
        m_system_manager.addSystem<NameSystem>();
        
        // 初始化系统
        m_system_manager.initialize(m_world_registry);
    }
    
    Entity World::createWorldSettingsEntity()
    {
        Entity world_entity = createWorldEntity("WorldSettings");
        addWorldComponent<WorldComponent>(world_entity, m_name, m_default_level_url);
        
        return world_entity;
    }
    
    // WorldManager 实现
    std::shared_ptr<World> WorldManager::createWorld(const std::string& name)
    {
        auto world = std::make_shared<World>(name);
        m_worlds[name] = world;
        
        if (m_current_world_name.empty())
        {
            m_current_world_name = name;
        }
        
        return world;
    }
    
    std::shared_ptr<World> WorldManager::getCurrentWorld()
    {
        if (m_current_world_name.empty() || m_worlds.find(m_current_world_name) == m_worlds.end())
        {
            return nullptr;
        }
        
        return m_worlds[m_current_world_name];
    }
    
    void WorldManager::setCurrentWorld(const std::string& name)
    {
        if (m_worlds.find(name) != m_worlds.end())
        {
            m_current_world_name = name;
        }
    }
    
    void WorldManager::destroyWorld(const std::string& name)
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
    
    bool WorldManager::loadWorldFromResource(const std::string& name, const WorldRes& world_res)
    {
        auto world = createWorld(name);
        return world->loadFromResource(world_res);
    }
    
    WorldRes WorldManager::saveWorldToResource(const std::string& name)
    {
        auto it = m_worlds.find(name);
        if (it != m_worlds.end())
        {
            return it->second->saveToResource();
        }
        
        return WorldRes();
    }
    
    void WorldManager::update(float delta_time)
    {
        if (auto current_world = getCurrentWorld())
        {
            current_world->update(delta_time);
        }
    }
    
    std::vector<std::string> WorldManager::getAllWorldNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : m_worlds)
        {
            names.push_back(pair.first);
        }
        return names;
    }
    
    void WorldManager::clearAllWorlds()
    {
        for (auto& pair : m_worlds)
        {
            pair.second->shutdown();
        }
        m_worlds.clear();
        m_current_world_name.clear();
    }
    
} // namespace Piccolo
