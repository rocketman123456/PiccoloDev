#pragma once
#include "runtime/core/math/matrix4.h"
#include "runtime/core/math/quaternion.h"
#include "runtime/core/math/vector3.h"
#include "runtime/function/ecs/types.h"
#include "runtime/function/framework/component/component.h"
#include "runtime/resource/res_type/components/lua_script.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace Piccolo
{
    // 基础组件
    struct TagComponent
    {
        std::string tag;

        TagComponent() = default;
        TagComponent(const std::string& t)
            : tag(t)
        {}
    };

    // 名称和激活状态组件
    struct NameComponent
    {
        std::string name;
        bool        active = true;

        NameComponent() = default;
        NameComponent(const std::string& n, bool a = true)
            : name(n)
            , active(a)
        {}
    };

    // 变换组件
    struct TransformComponent
    {
        Vector3    position         = Vector3::ZERO;
        Quaternion rotation         = Quaternion::IDENTITY;
        Vector3    scale            = Vector3::UNIT_SCALE;
        Matrix4x4  transform_matrix = Matrix4x4::IDENTITY;
        bool       dirty            = true;

        TransformComponent() = default;
        TransformComponent(const Vector3& pos, const Quaternion& rot = Quaternion::IDENTITY, const Vector3& scl = Vector3::UNIT_SCALE)
            : position(pos)
            , rotation(rot)
            , scale(scl)
            , dirty(true)
        {}

        void updateTransformMatrix()
        {
            if (dirty)
            {
                transform_matrix = Matrix4x4::IDENTITY;
                transform_matrix.setTrans(position);
                // TODO: 实现旋转设置
                // transform_matrix.setOrientation(rotation);
                transform_matrix.setScale(scale);
                dirty = false;
            }
        }
    };

    // 与现有GObject系统的兼容性组件
    struct ObjectIDComponent
    {
        size_t object_id;

        ObjectIDComponent() = default;
        ObjectIDComponent(size_t id)
            : object_id(id)
        {}
    };

    // 关卡引用组件
    struct LevelComponent
    {
        std::string level_name;
        std::string level_url;

        LevelComponent() = default;
        LevelComponent(const std::string& name, const std::string& url)
            : level_name(name)
            , level_url(url)
        {}
    };

    // 世界引用组件
    struct WorldComponent
    {
        std::string world_name;
        std::string world_url;

        WorldComponent() = default;
        WorldComponent(const std::string& name, const std::string& url)
            : world_name(name)
            , world_url(url)
        {}
    };

    // 可渲染对象组件
    struct RenderableComponent
    {
        std::string mesh_url;
        std::string material_url;
        bool        visible        = true;
        bool        cast_shadow    = true;
        bool        receive_shadow = true;

        RenderableComponent() = default;
        RenderableComponent(const std::string& mesh, const std::string& material)
            : mesh_url(mesh)
            , material_url(material)
        {}
    };

    // 相机组件
    struct CameraComponent
    {
        float     fov               = 45.0f;
        float     near_plane        = 0.1f;
        float     far_plane         = 1000.0f;
        bool      is_main_camera    = false;
        Matrix4x4 view_matrix       = Matrix4x4::IDENTITY;
        Matrix4x4 projection_matrix = Matrix4x4::IDENTITY;

        CameraComponent() = default;
        CameraComponent(float f, float near_p, float far_p, bool main = false)
            : fov(f)
            , near_plane(near_p)
            , far_plane(far_p)
            , is_main_camera(main)
        {}
    };

    // 光源组件
    struct LightComponent
    {
        enum class LightType
        {
            Directional,
            Point,
            Spot
        };

        LightType type             = LightType::Directional;
        Vector3   color            = Vector3::UNIT_SCALE;
        float     intensity        = 1.0f;
        float     range            = 10.0f;
        float     inner_cone_angle = 30.0f;
        float     outer_cone_angle = 45.0f;
        bool      cast_shadow      = true;

        LightComponent() = default;
        LightComponent(LightType t, const Vector3& c, float i)
            : type(t)
            , color(c)
            , intensity(i)
        {}
    };

    // Lua脚本组件
    struct LuaScriptComponent
    {
        std::shared_ptr<LuaScriptRes> script_resource;
        std::string                   script_url;
        bool                          enabled = true;

        LuaScriptComponent() = default;
        LuaScriptComponent(const std::string& url)
            : script_url(url)
        {}
    };

    // 组件定义资源组件（用于序列化）
    struct ComponentDefinitionComponent
    {
        std::string type_name;
        std::string component_data;

        ComponentDefinitionComponent() = default;
        ComponentDefinitionComponent(const std::string& type, const std::string& data)
            : type_name(type)
            , component_data(data)
        {}
    };

} // namespace Piccolo