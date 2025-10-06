#pragma once
#include "types.h"
#include "components.h"
#include "entt_coordinator.h"
#include "runtime/core/meta/reflection/reflection.h"
#include "runtime/resource/res_type/common/object.h"
#include "runtime/resource/res_type/common/level.h"
#include "runtime/resource/res_type/common/world.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Piccolo
{
    // ECS序列化器
    class ECSSerializer
    {
    public:
        ECSSerializer() = default;
        ~ECSSerializer() = default;
        
        // 从资源创建实体
        Entity createEntityFromObjectInstance(const ObjectInstanceRes& object_instance, Registry& registry);
        Entity createEntityFromComponentDefinition(const ComponentDefinitionRes& component_def, Registry& registry);
        
        // 序列化实体到资源
        ObjectInstanceRes serializeEntityToObjectInstance(Entity entity, const Registry& registry);
        ComponentDefinitionRes serializeEntityToComponentDefinition(Entity entity, const Registry& registry);
        
        // 批量操作
        std::vector<Entity> createEntitiesFromLevel(const LevelRes& level_res, Registry& registry);
        LevelRes serializeLevelFromEntities(const Registry& registry, const std::string& level_name);
        
        std::vector<Entity> createEntitiesFromWorld(const WorldRes& world_res, Registry& registry);
        WorldRes serializeWorldFromEntities(const Registry& registry, const std::string& world_name);
        
        // 组件类型映射
        void registerComponentType(const std::string& type_name, std::function<void(Entity, const std::string&, Registry&)> creator);
        
    private:
        // 组件创建函数映射
        std::unordered_map<std::string, std::function<void(Entity, const std::string&, Registry&)>> m_component_creators;
        
        // 辅助函数
        void addComponentFromString(Entity entity, const std::string& type_name, const std::string& data, Registry& registry);
        std::string serializeComponentToString(Entity entity, const std::string& type_name, const Registry& registry);
        
        // 初始化默认组件类型
        void initializeDefaultComponentTypes();
    };
    
    // 资源到ECS的转换器
    class ResourceToECSConverter
    {
    public:
        ResourceToECSConverter() = default;
        ~ResourceToECSConverter() = default;
        
        // 转换对象实例资源到ECS实体
        Entity convertObjectInstanceToEntity(const ObjectInstanceRes& object_instance, Registry& registry);
        
        // 转换组件定义资源到ECS组件
        template<typename ComponentType>
        void convertComponentDefinitionToComponent(const ComponentDefinitionRes& component_def, Entity entity, Registry& registry);
        
        // 转换关卡资源到ECS实体集合
        std::vector<Entity> convertLevelToEntities(const LevelRes& level_res, Registry& registry);
        
        // 转换世界资源到ECS实体集合
        std::vector<Entity> convertWorldToEntities(const WorldRes& world_res, Registry& registry);
        
    private:
        ECSSerializer m_serializer;
    };
    
    // ECS到资源的转换器
    class ECSToResourceConverter
    {
    public:
        ECSToResourceConverter() = default;
        ~ECSToResourceConverter() = default;
        
        // 转换ECS实体到对象实例资源
        ObjectInstanceRes convertEntityToObjectInstance(Entity entity, const Registry& registry);
        
        // 转换ECS组件到组件定义资源
        template<typename ComponentType>
        ComponentDefinitionRes convertComponentToComponentDefinition(Entity entity, const Registry& registry);
        
        // 转换ECS实体集合到关卡资源
        LevelRes convertEntitiesToLevel(const Registry& registry, const std::string& level_name);
        
        // 转换ECS实体集合到世界资源
        WorldRes convertEntitiesToWorld(const Registry& registry, const std::string& world_name);
        
    private:
        ECSSerializer m_serializer;
    };
    
    // 全局转换器实例
    inline ResourceToECSConverter& getResourceToECSConverter()
    {
        static ResourceToECSConverter converter;
        return converter;
    }
    
    inline ECSToResourceConverter& getECSToResourceConverter()
    {
        static ECSToResourceConverter converter;
        return converter;
    }
    
} // namespace Piccolo
