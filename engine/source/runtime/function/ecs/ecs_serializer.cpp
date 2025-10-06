#include "ecs_serializer.h"
#include "runtime/core/meta/reflection/reflection.h"
#include "runtime/resource/res_type/components/lua_script.h"

#include <sstream>
#include <fstream>

namespace Piccolo
{
    // ECSSerializer 实现
    Entity ECSSerializer::createEntityFromObjectInstance(const ObjectInstanceRes& object_instance, Registry& registry)
    {
        Entity entity = registry.create();
        
        // 添加基础组件
        registry.emplace<NameComponent>(entity, object_instance.m_name, object_instance.m_active);
        
        // 添加组件定义组件
        for (const auto& component_ptr : object_instance.m_instanced_components)
        {
            if (component_ptr)
            {
                // 这里需要根据组件类型创建相应的ECS组件
                // 由于组件是反射系统管理的，我们需要通过反射来获取类型信息
                ComponentDefinitionComponent comp_def;
                comp_def.type_name = typeid(*component_ptr).name(); // 使用typeid获取类型名
                comp_def.component_data = ""; // 需要序列化组件数据
                registry.emplace<ComponentDefinitionComponent>(entity, comp_def);
            }
        }
        
        return entity;
    }
    
    Entity ECSSerializer::createEntityFromComponentDefinition(const ComponentDefinitionRes& component_def, Registry& registry)
    {
        Entity entity = registry.create();
        
        // 根据组件类型名称创建相应的组件
        addComponentFromString(entity, component_def.m_type_name, component_def.m_component, registry);
        
        return entity;
    }
    
    ObjectInstanceRes ECSSerializer::serializeEntityToObjectInstance(Entity entity, const Registry& registry)
    {
        ObjectInstanceRes object_instance;
        
        // 获取名称组件
        if (registry.all_of<NameComponent>(entity))
        {
            const auto& name_comp = registry.get<NameComponent>(entity);
            object_instance.m_name = name_comp.name;
            object_instance.m_active = name_comp.active;
        }
        
        // 序列化所有组件定义组件
        if (registry.all_of<ComponentDefinitionComponent>(entity))
        {
            const auto& comp_def = registry.get<ComponentDefinitionComponent>(entity);
            ComponentDefinitionRes component_def_res;
            component_def_res.m_type_name = comp_def.type_name;
            component_def_res.m_component = comp_def.component_data;
            // 这里需要将ComponentDefinitionRes转换为ReflectionPtr<Component>
            // 由于涉及反射系统，这里暂时留空
        }
        
        return object_instance;
    }
    
    ComponentDefinitionRes ECSSerializer::serializeEntityToComponentDefinition(Entity entity, const Registry& registry)
    {
        ComponentDefinitionRes component_def;
        
        if (registry.all_of<ComponentDefinitionComponent>(entity))
        {
            const auto& comp_def = registry.get<ComponentDefinitionComponent>(entity);
            component_def.m_type_name = comp_def.type_name;
            component_def.m_component = comp_def.component_data;
        }
        
        return component_def;
    }
    
    std::vector<Entity> ECSSerializer::createEntitiesFromLevel(const LevelRes& level_res, Registry& registry)
    {
        std::vector<Entity> entities;
        
        for (const auto& object_instance : level_res.m_objects)
        {
            Entity entity = createEntityFromObjectInstance(object_instance, registry);
            entities.push_back(entity);
        }
        
        return entities;
    }
    
    LevelRes ECSSerializer::serializeLevelFromEntities(const Registry& registry, const std::string& level_name)
    {
        LevelRes level_res;
        
        // 获取所有有LevelComponent的实体
        auto view = registry.view<LevelComponent>();
        for (auto entity : view)
        {
            const auto& level_comp = view.get<LevelComponent>(entity);
            if (level_comp.level_name == level_name)
            {
                ObjectInstanceRes object_instance = serializeEntityToObjectInstance(entity, registry);
                level_res.m_objects.push_back(object_instance);
            }
        }
        
        return level_res;
    }
    
    std::vector<Entity> ECSSerializer::createEntitiesFromWorld(const WorldRes& world_res, Registry& registry)
    {
        std::vector<Entity> entities;
        
        // 创建世界级实体
        Entity world_entity = registry.create();
        registry.emplace<WorldComponent>(world_entity, world_res.m_name, world_res.m_default_level_url);
        registry.emplace<NameComponent>(world_entity, world_res.m_name);
        entities.push_back(world_entity);
        
        return entities;
    }
    
    WorldRes ECSSerializer::serializeWorldFromEntities(const Registry& registry, const std::string& world_name)
    {
        WorldRes world_res;
        world_res.m_name = world_name;
        
        // 获取世界组件
        auto view = registry.view<WorldComponent>();
        for (auto entity : view)
        {
            const auto& world_comp = view.get<WorldComponent>(entity);
            if (world_comp.world_name == world_name)
            {
                world_res.m_default_level_url = world_comp.world_url;
                break;
            }
        }
        
        return world_res;
    }
    
    void ECSSerializer::registerComponentType(const std::string& type_name, std::function<void(Entity, const std::string&, Registry&)> creator)
    {
        m_component_creators[type_name] = creator;
    }
    
    void ECSSerializer::addComponentFromString(Entity entity, const std::string& type_name, const std::string& data, Registry& registry)
    {
        auto it = m_component_creators.find(type_name);
        if (it != m_component_creators.end())
        {
            it->second(entity, data, registry);
        }
    }
    
    std::string ECSSerializer::serializeComponentToString(Entity entity, const std::string& type_name, const Registry& registry)
    {
        // 这里需要根据组件类型进行序列化
        // 由于涉及反射系统，这里暂时返回空字符串
        return "";
    }
    
    void ECSSerializer::initializeDefaultComponentTypes()
    {
        // 注册默认组件类型
        registerComponentType("TransformComponent", [](Entity entity, const std::string& data, Registry& registry) {
            registry.emplace<TransformComponent>(entity);
        });
        
        registerComponentType("RenderableComponent", [](Entity entity, const std::string& data, Registry& registry) {
            registry.emplace<RenderableComponent>(entity);
        });
        
        registerComponentType("CameraComponent", [](Entity entity, const std::string& data, Registry& registry) {
            registry.emplace<CameraComponent>(entity);
        });
        
        registerComponentType("LightComponent", [](Entity entity, const std::string& data, Registry& registry) {
            registry.emplace<LightComponent>(entity);
        });
        
        registerComponentType("LuaScriptComponent", [](Entity entity, const std::string& data, Registry& registry) {
            registry.emplace<LuaScriptComponent>(entity);
        });
    }
    
    // ResourceToECSConverter 实现
    Entity ResourceToECSConverter::convertObjectInstanceToEntity(const ObjectInstanceRes& object_instance, Registry& registry)
    {
        return m_serializer.createEntityFromObjectInstance(object_instance, registry);
    }
    
    std::vector<Entity> ResourceToECSConverter::convertLevelToEntities(const LevelRes& level_res, Registry& registry)
    {
        return m_serializer.createEntitiesFromLevel(level_res, registry);
    }
    
    std::vector<Entity> ResourceToECSConverter::convertWorldToEntities(const WorldRes& world_res, Registry& registry)
    {
        return m_serializer.createEntitiesFromWorld(world_res, registry);
    }
    
    // ECSToResourceConverter 实现
    ObjectInstanceRes ECSToResourceConverter::convertEntityToObjectInstance(Entity entity, const Registry& registry)
    {
        return m_serializer.serializeEntityToObjectInstance(entity, registry);
    }
    
    LevelRes ECSToResourceConverter::convertEntitiesToLevel(const Registry& registry, const std::string& level_name)
    {
        return m_serializer.serializeLevelFromEntities(registry, level_name);
    }
    
    WorldRes ECSToResourceConverter::convertEntitiesToWorld(const Registry& registry, const std::string& world_name)
    {
        return m_serializer.serializeWorldFromEntities(registry, world_name);
    }
    
} // namespace Piccolo
