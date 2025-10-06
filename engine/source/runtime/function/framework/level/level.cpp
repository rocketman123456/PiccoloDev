#include "level.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/framework/object/gobject.h"

#include <algorithm>

namespace Piccolo
{
    // Level 实现
    Level::Level(const std::string& name) : m_name(name)
    {
        initialize();
    }
    
    void Level::initialize()
    {
        // 初始化默认系统
        initializeDefaultSystems();
        
        // 创建关卡设置实体
        createLevelSettingsEntity();
    }
    
    void Level::shutdown()
    {
        // 关闭所有系统
        m_system_manager.shutdown(m_level_registry);
        
        // 清空GObject
        m_gobjects.clear();
        
        // 清空映射
        m_object_id_to_entity.clear();
        m_entity_to_object_id.clear();
        m_entity_names.clear();
        
        // 清空注册表
        m_level_registry.clear();
    }
    
    void Level::update(float delta_time)
    {
        // 更新所有系统
        m_system_manager.update(m_level_registry, delta_time);
        
        // 更新所有GObject
        for (auto& pair : m_gobjects)
        {
            if (pair.second)
            {
                // 这里应该调用GObject的tick方法
                // pair.second->tick(delta_time);
            }
        }
    }
    
    bool Level::loadFromResource(const LevelRes& level_res)
    {
        // 使用转换器从资源创建实体
        auto& converter = getResourceToECSConverter();
        auto entities = converter.convertLevelToEntities(level_res, m_level_registry);
        
        // 为每个实体创建对应的GObject
        for (auto entity : entities)
        {
            if (m_level_registry.all_of<NameComponent>(entity))
            {
                const auto& name_comp = m_level_registry.get<NameComponent>(entity);
                auto gobject = createGObject(name_comp.name);
                
                // 建立映射关系
                size_t object_id = gobject->getID(); // 假设GObject有getID方法
                m_object_id_to_entity[object_id] = entity;
                m_entity_to_object_id[entity] = object_id;
            }
        }
        
        // 添加PostLoad任务
        addPostLoadTasks();
        
        return true;
    }
    
    LevelRes Level::saveToResource() const
    {
        LevelRes level_res;
        
        // 使用转换器将实体保存为资源
        auto& converter = getECSToResourceConverter();
        level_res = converter.convertEntitiesToLevel(m_level_registry, m_name);
        
        return level_res;
    }
    
    Entity Level::createEntity(const std::string& name)
    {
        Entity entity = m_level_registry.create();
        
        if (!name.empty())
        {
            addComponent<NameComponent>(entity, name);
            m_entity_names[name] = entity;
        }
        
        return entity;
    }
    
    void Level::destroyEntity(Entity entity)
    {
        // 从映射中移除
        auto entity_it = m_entity_to_object_id.find(entity);
        if (entity_it != m_entity_to_object_id.end())
        {
            size_t object_id = entity_it->second;
            m_object_id_to_entity.erase(object_id);
            m_entity_to_object_id.erase(entity_it);
            
            // 销毁对应的GObject
            auto gobject_it = m_gobjects.find(object_id);
            if (gobject_it != m_gobjects.end())
            {
                m_gobjects.erase(gobject_it);
            }
        }
        
        // 从名称映射中移除
        if (m_level_registry.all_of<NameComponent>(entity))
        {
            const auto& name_comp = m_level_registry.get<NameComponent>(entity);
            auto name_it = m_entity_names.find(name_comp.name);
            if (name_it != m_entity_names.end())
            {
                m_entity_names.erase(name_it);
            }
        }
        
        m_level_registry.destroy(entity);
    }
    
    std::shared_ptr<GObject> Level::createGObject(const std::string& name)
    {
        size_t object_id = generateObjectID();
        auto gobject = std::make_shared<GObject>(object_id, name); // 假设GObject构造函数接受ID和名称
        
        m_gobjects[object_id] = gobject;
        return gobject;
    }
    
    void Level::destroyGObject(std::shared_ptr<GObject> gobject)
    {
        if (!gobject)
            return;
        
        size_t object_id = gobject->getID(); // 假设GObject有getID方法
        
        // 销毁对应的实体
        auto entity_it = m_object_id_to_entity.find(object_id);
        if (entity_it != m_object_id_to_entity.end())
        {
            destroyEntity(entity_it->second);
        }
        
        // 从GObject映射中移除
        m_gobjects.erase(object_id);
    }
    
    std::shared_ptr<GObject> Level::getGObject(size_t object_id)
    {
        auto it = m_gobjects.find(object_id);
        if (it != m_gobjects.end())
        {
            return it->second;
        }
        return nullptr;
    }
    
    Entity Level::getEntityByObjectID(size_t object_id) const
    {
        auto it = m_object_id_to_entity.find(object_id);
        if (it != m_object_id_to_entity.end())
        {
            return it->second;
        }
        return INVALID_ENTITY;
    }
    
    size_t Level::getObjectIDByEntity(Entity entity) const
    {
        auto it = m_entity_to_object_id.find(entity);
        if (it != m_entity_to_object_id.end())
        {
            return it->second;
        }
        return 0; // 无效ID
    }
    
    void Level::executePostLoadTasks()
    {
        auto& postload_manager = getPostLoadManager();
        postload_manager.executeAllPostLoadTasks();
    }
    
    void Level::addPostLoadTasks()
    {
        auto& postload_manager = getPostLoadManager();
        postload_manager.addPostLoadTasksForRegistry(m_level_registry);
    }
    
    void Level::addSystem(std::unique_ptr<System> system)
    {
        m_system_manager.addSystem(std::move(system));
    }
    
    void Level::removeSystem(const std::string& system_name)
    {
        // 这里需要根据系统名称移除系统，但System基类没有名称
        // 可以考虑为System添加名称成员或者使用typeid
    }
    
    System* Level::getSystem(const std::string& system_name)
    {
        // 这里需要根据系统名称获取系统，但System基类没有名称
        // 可以考虑为System添加名称成员或者使用typeid
        return nullptr;
    }
    
    void Level::initializeDefaultSystems()
    {
        // 添加默认系统
        m_system_manager.addSystem<TransformSystem>();
        m_system_manager.addSystem<RenderSystem>();
        m_system_manager.addSystem<CameraSystem>();
        m_system_manager.addSystem<LightSystem>();
        m_system_manager.addSystem<NameSystem>();
        m_system_manager.addSystem<LuaScriptSystem>();
        
        // 初始化系统
        m_system_manager.initialize(m_level_registry);
    }
    
    Entity Level::createLevelSettingsEntity()
    {
        Entity level_entity = createEntity("LevelSettings");
        addComponent<LevelComponent>(level_entity, m_name, "");
        
        return level_entity;
    }
    
    size_t Level::generateObjectID()
    {
        return m_next_object_id++;
    }
    
    // LevelManager 实现
    std::shared_ptr<Level> LevelManager::createLevel(const std::string& name)
    {
        auto level = std::make_shared<Level>(name);
        m_levels[name] = level;
        
        if (m_current_level_name.empty())
        {
            m_current_level_name = name;
        }
        
        return level;
    }
    
    std::shared_ptr<Level> LevelManager::getCurrentLevel()
    {
        if (m_current_level_name.empty() || m_levels.find(m_current_level_name) == m_levels.end())
        {
            return nullptr;
        }
        
        return m_levels[m_current_level_name];
    }
    
    void LevelManager::setCurrentLevel(const std::string& name)
    {
        if (m_levels.find(name) != m_levels.end())
        {
            m_current_level_name = name;
        }
    }
    
    void LevelManager::destroyLevel(const std::string& name)
    {
        auto it = m_levels.find(name);
        if (it != m_levels.end())
        {
            it->second->shutdown();
            m_levels.erase(it);
            
            if (m_current_level_name == name)
            {
                m_current_level_name.clear();
            }
        }
    }
    
    bool LevelManager::loadLevelFromResource(const std::string& name, const LevelRes& level_res)
    {
        auto level = createLevel(name);
        return level->loadFromResource(level_res);
    }
    
    LevelRes LevelManager::saveLevelToResource(const std::string& name)
    {
        auto it = m_levels.find(name);
        if (it != m_levels.end())
        {
            return it->second->saveToResource();
        }
        
        return LevelRes();
    }
    
    void LevelManager::update(float delta_time)
    {
        if (auto current_level = getCurrentLevel())
        {
            current_level->update(delta_time);
        }
    }
    
    std::vector<std::string> LevelManager::getAllLevelNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : m_levels)
        {
            names.push_back(pair.first);
        }
        return names;
    }
    
    void LevelManager::clearAllLevels()
    {
        for (auto& pair : m_levels)
        {
            pair.second->shutdown();
        }
        m_levels.clear();
        m_current_level_name.clear();
    }
    
} // namespace Piccolo
