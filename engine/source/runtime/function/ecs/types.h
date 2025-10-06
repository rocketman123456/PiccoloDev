#pragma once
#include <entt/entt.hpp>
#include <string>

namespace Piccolo
{
    // ECS 类型定义
    using Entity = entt::entity;
    using Registry = entt::registry;
    
    // 事件系统类型定义
    using EventDispatcher = entt::dispatcher;
    
    // 实体无效值
    constexpr Entity INVALID_ENTITY = entt::null;
    
    // 组件标签
    struct TagComponent;
    struct NameComponent;
    struct TransformComponent;
    struct ObjectIDComponent;
    struct LevelComponent;
    struct WorldComponent;
    struct RenderableComponent;
    struct CameraComponent;
    struct LightComponent;
    struct LuaScriptComponent;
    
} // namespace Piccolo