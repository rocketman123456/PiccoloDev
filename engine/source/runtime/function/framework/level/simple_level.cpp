#include "simple_level.h"

namespace Piccolo
{
    // SimpleLevel 实现
    SimpleLevel::SimpleLevel(const std::string& name) : m_name(name)
    {
        initialize();
    }
    
    void SimpleLevel::initialize()
    {
        // 初始化默认系统
        initializeDefaultSystems();
        
        // 创建关卡设置实体
        createLevelSettingsEntity();
    }
    
    void SimpleLevel::shutdown()
    {
        // 关闭所有系统
        m_system_manager.shutdown(m_entity_manager);
        
        // 清空实体管理器
        // EntityManager会在析构时自动清理
    }
    
    void SimpleLevel::update(float delta_time)
    {
        // 更新所有系统
        m_system_manager.update(m_entity_manager, delta_time);
    }
    
    bool SimpleLevel::loadFromResource(const LevelRes& level_res)
    {
        // 从资源创建实体
        for (const auto& object_instance : level_res.m_objects)
        {
            createEntityFromObjectInstance(object_instance);
        }
        
        // 执行PostLoad
        executePostLoad();
        
        return true;
    }
    
    LevelRes SimpleLevel::saveToResource() const
    {
        LevelRes level_res;
        
        // 将所有实体序列化为对象实例资源
        auto entities = m_entity_manager.getAllEntities();
        for (const auto* entity : entities)
        {
            ObjectInstanceRes object_instance = serializeEntityToObjectInstance(entity->getID());
            level_res.m_objects.push_back(object_instance);
        }
        
        return level_res;
    }
    
    Entity* SimpleLevel::createEntity(const std::string& name)
    {
        Entity* entity = m_entity_manager.createEntity();
        
        if (!name.empty())
        {
            entity->addComponent<NameComponent>(name);
            m_entity_names[name] = entity->getID();
        }
        
        return entity;
    }
    
    void SimpleLevel::destroyEntity(EntityID entity_id)
    {
        // 从名称映射中移除
        for (auto it = m_entity_names.begin(); it != m_entity_names.end(); ++it)
        {
            if (it->second == entity_id)
            {
                m_entity_names.erase(it);
                break;
            }
        }
        
        m_entity_manager.destroyEntity(entity_id);
    }
    
    std::vector<Entity*> SimpleLevel::getEntitiesWithComponent(const std::string& component_type)
    {
        std::vector<Entity*> result;
        auto entities = m_entity_manager.getAllEntities();
        
        for (auto* entity : entities)
        {
            // 这里需要根据组件类型名称检查组件
            // 由于我们使用的是typeid，这里简化处理
            if (component_type == "TransformComponent" && entity->hasComponent<TransformComponent>())
            {
                result.push_back(entity);
            }
            else if (component_type == "RenderableComponent" && entity->hasComponent<RenderableComponent>())
            {
                result.push_back(entity);
            }
            else if (component_type == "CameraComponent" && entity->hasComponent<CameraComponent>())
            {
                result.push_back(entity);
            }
            else if (component_type == "LightComponent" && entity->hasComponent<LightComponent>())
            {
                result.push_back(entity);
            }
        }
        
        return result;
    }
    
    std::vector<Entity*> SimpleLevel::getAllEntities()
    {
        return m_entity_manager.getAllEntities();
    }
    
    void SimpleLevel::executePostLoad()
    {
        m_entity_manager.postLoad();
    }
    
    void SimpleLevel::addSystem(std::unique_ptr<System> system)
    {
        m_system_manager.addSystem(std::move(system));
    }
    
    Entity* SimpleLevel::createEntityFromObjectInstance(const ObjectInstanceRes& object_instance)
    {
        Entity* entity = createEntity(object_instance.m_name);
        
        if (entity)
        {
            // 添加基础组件
            entity->addComponent<NameComponent>(object_instance.m_name, object_instance.m_active);
            
            // 这里可以根据object_instance.m_instanced_components添加更多组件
            // 由于涉及反射系统，这里简化处理
        }
        
        return entity;
    }
    
    ObjectInstanceRes SimpleLevel::serializeEntityToObjectInstance(EntityID entity_id) const
    {
        ObjectInstanceRes object_instance;
        
        const Entity* entity = m_entity_manager.getEntity(entity_id);
        if (entity)
        {
            // 获取名称组件
            if (auto* name_comp = entity->getComponent<NameComponent>())
            {
                object_instance.m_name = name_comp->name;
                object_instance.m_active = name_comp->active;
            }
            
            // 这里可以序列化其他组件
            // 由于涉及反射系统，这里简化处理
        }
        
        return object_instance;
    }
    
    void SimpleLevel::initializeDefaultSystems()
    {
        // 添加默认系统
        m_system_manager.addSystem(std::make_unique<TransformSystem>());
        m_system_manager.addSystem(std::make_unique<RenderSystem>());
        
        // 初始化系统
        m_system_manager.initialize(m_entity_manager);
    }
    
    Entity* SimpleLevel::createLevelSettingsEntity()
    {
        Entity* level_entity = createEntity("LevelSettings");
        return level_entity;
    }
    
    // SimpleLevelManager 实现
    std::shared_ptr<SimpleLevel> SimpleLevelManager::createLevel(const std::string& name)
    {
        auto level = std::make_shared<SimpleLevel>(name);
        m_levels[name] = level;
        
        if (m_current_level_name.empty())
        {
            m_current_level_name = name;
        }
        
        return level;
    }
    
    std::shared_ptr<SimpleLevel> SimpleLevelManager::getCurrentLevel()
    {
        if (m_current_level_name.empty() || m_levels.find(m_current_level_name) == m_levels.end())
        {
            return nullptr;
        }
        
        return m_levels[m_current_level_name];
    }
    
    void SimpleLevelManager::setCurrentLevel(const std::string& name)
    {
        if (m_levels.find(name) != m_levels.end())
        {
            m_current_level_name = name;
        }
    }
    
    void SimpleLevelManager::destroyLevel(const std::string& name)
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
    
    bool SimpleLevelManager::loadLevelFromResource(const std::string& name, const LevelRes& level_res)
    {
        auto level = createLevel(name);
        return level->loadFromResource(level_res);
    }
    
    LevelRes SimpleLevelManager::saveLevelToResource(const std::string& name)
    {
        auto it = m_levels.find(name);
        if (it != m_levels.end())
        {
            return it->second->saveToResource();
        }
        
        return LevelRes();
    }
    
    void SimpleLevelManager::update(float delta_time)
    {
        if (auto current_level = getCurrentLevel())
        {
            current_level->update(delta_time);
        }
    }
    
    std::vector<std::string> SimpleLevelManager::getAllLevelNames() const
    {
        std::vector<std::string> names;
        for (const auto& pair : m_levels)
        {
            names.push_back(pair.first);
        }
        return names;
    }
    
    void SimpleLevelManager::clearAllLevels()
    {
        for (auto& pair : m_levels)
        {
            pair.second->shutdown();
        }
        m_levels.clear();
        m_current_level_name.clear();
    }
    
} // namespace Piccolo
