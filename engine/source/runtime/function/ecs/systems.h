#pragma once
#include "types.h"
#include "components.h"
#include "entt_coordinator.h"

#include <vector>
#include <algorithm>

namespace Piccolo
{
    // 基础系统类
    class System
    {
    public:
        virtual ~System() = default;
        virtual void update(Registry& registry, float delta_time) = 0;
        virtual void initialize(Registry& registry) {}
        virtual void shutdown(Registry& registry) {}
    };
    
    // 变换系统
    class TransformSystem : public System
    {
    public:
        void update(Registry& registry, float delta_time) override
        {
            auto view = registry.view<TransformComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                if (transform.dirty)
                {
                    transform.updateTransformMatrix();
                }
            }
        }
    };
    
    // 渲染系统
    class RenderSystem : public System
    {
    public:
        void update(Registry& registry, float delta_time) override
        {
            // 获取所有可渲染的实体
            auto view = registry.view<TransformComponent, RenderableComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                auto& renderable = view.get<RenderableComponent>(entity);
                
                if (renderable.visible)
                {
                    // 更新变换矩阵
                    if (transform.dirty)
                    {
                        transform.updateTransformMatrix();
                    }
                    
                    // 这里可以添加渲染逻辑
                    // 例如：提交渲染命令、更新渲染状态等
                }
            }
        }
    };
    
    // 相机系统
    class CameraSystem : public System
    {
    public:
        void update(Registry& registry, float delta_time) override
        {
            auto view = registry.view<TransformComponent, CameraComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                auto& camera = view.get<CameraComponent>(entity);
                
                // 更新视图矩阵
                updateViewMatrix(transform, camera);
                
                // 更新投影矩阵
                updateProjectionMatrix(camera);
            }
        }
        
    private:
        void updateViewMatrix(const TransformComponent& transform, CameraComponent& camera)
        {
            // 基于变换组件更新视图矩阵
            Vector3 forward = transform.rotation * Vector3::NEGATIVE_UNIT_Z;
            Vector3 up = transform.rotation * Vector3::UNIT_Y;
            
            // 这里应该实现视图矩阵的计算
            // camera.view_matrix = Matrix4x4::lookAt(transform.position, transform.position + forward, up);
        }
        
        void updateProjectionMatrix(CameraComponent& camera)
        {
            // 这里应该实现投影矩阵的计算
            // camera.projection_matrix = Matrix4x4::perspective(camera.fov, aspect_ratio, camera.near_plane, camera.far_plane);
        }
    };
    
    // 光照系统
    class LightSystem : public System
    {
    public:
        void update(Registry& registry, float delta_time) override
        {
            auto view = registry.view<TransformComponent, LightComponent>();
            for (auto entity : view)
            {
                auto& transform = view.get<TransformComponent>(entity);
                auto& light = view.get<LightComponent>(entity);
                
                // 更新光照数据
                updateLightData(transform, light);
            }
        }
        
    private:
        void updateLightData(const TransformComponent& transform, const LightComponent& light)
        {
            // 根据光源类型更新光照数据
            switch (light.type)
            {
                case LightComponent::LightType::Directional:
                    // 方向光：使用旋转方向
                    break;
                case LightComponent::LightType::Point:
                    // 点光源：使用位置
                    break;
                case LightComponent::LightType::Spot:
                    // 聚光灯：使用位置和方向
                    break;
            }
        }
    };
    
    // 名称和标签系统
    class NameSystem : public System
    {
    public:
        void update(Registry& registry, float delta_time) override
        {
            // 处理名称和标签的更新逻辑
            auto name_view = registry.view<NameComponent>();
            for (auto entity : name_view)
            {
                auto& name = name_view.get<NameComponent>(entity);
                // 处理名称相关的逻辑
            }
            
            auto tag_view = registry.view<TagComponent>();
            for (auto entity : tag_view)
            {
                auto& tag = tag_view.get<TagComponent>(entity);
                // 处理标签相关的逻辑
            }
        }
    };
    
    // Lua脚本系统
    class LuaScriptSystem : public System
    {
    public:
        void update(Registry& registry, float delta_time) override
        {
            auto view = registry.view<LuaScriptComponent>();
            for (auto entity : view)
            {
                auto& script = view.get<LuaScriptComponent>(entity);
                
                if (script.enabled && script.script_resource)
                {
                    // 执行Lua脚本更新逻辑
                    // 这里需要集成Lua脚本执行器
                }
            }
        }
    };
    
    // 系统管理器
    class SystemManager
    {
    public:
        SystemManager() = default;
        ~SystemManager() = default;
        
        // 添加系统
        template<typename SystemType, typename... Args>
        void addSystem(Args&&... args)
        {
            static_assert(std::is_base_of_v<System, SystemType>, "SystemType must inherit from System");
            auto system = std::make_unique<SystemType>(std::forward<Args>(args)...);
            m_systems.push_back(std::move(system));
        }
        
        // 直接添加已创建的系统
        void addSystem(std::unique_ptr<System> system)
        {
            m_systems.push_back(std::move(system));
        }
        
        // 移除系统
        template<typename SystemType>
        void removeSystem()
        {
            m_systems.erase(
                std::remove_if(m_systems.begin(), m_systems.end(),
                    [](const std::unique_ptr<System>& system) {
                        return dynamic_cast<SystemType*>(system.get()) != nullptr;
                    }),
                m_systems.end()
            );
        }
        
        // 更新所有系统
        void update(Registry& registry, float delta_time)
        {
            for (auto& system : m_systems)
            {
                system->update(registry, delta_time);
            }
        }
        
        // 初始化所有系统
        void initialize(Registry& registry)
        {
            for (auto& system : m_systems)
            {
                system->initialize(registry);
            }
        }
        
        // 关闭所有系统
        void shutdown(Registry& registry)
        {
            for (auto& system : m_systems)
            {
                system->shutdown(registry);
            }
        }
        
        // 清空所有系统
        void clear()
        {
            m_systems.clear();
        }
        
    private:
        std::vector<std::unique_ptr<System>> m_systems;
    };
    
} // namespace Piccolo