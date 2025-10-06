#pragma once
#include "runtime/function/ecs/types.h"
#include "runtime/function/framework/component/component.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Piccolo
{
    // 前向声明
    class Component;
    
    // 游戏对象类 - 与ECS系统兼容的传统对象系统
    class GObject : public std::enable_shared_from_this<GObject>
    {
    public:
        GObject() = default;
        GObject(size_t id, const std::string& name = "");
        ~GObject() = default;
        
        // 禁用拷贝构造和赋值
        GObject(const GObject&) = delete;
        GObject& operator=(const GObject&) = delete;
        
        // 基本信息
        size_t getID() const { return m_id; }
        void setID(size_t id) { m_id = id; }
        
        std::string getName() const { return m_name; }
        void setName(const std::string& name) { m_name = name; }
        
        bool isActive() const { return m_active; }
        void setActive(bool active) { m_active = active; }
        
        // ECS集成
        Entity getECSEntity() const { return m_ecs_entity; }
        void setECSEntity(Entity entity) { m_ecs_entity = entity; }
        
        // 组件管理
        template<typename ComponentType, typename... Args>
        std::shared_ptr<ComponentType> addComponent(Args&&... args)
        {
            auto component = std::make_shared<ComponentType>(std::forward<Args>(args)...);
            component->postLoadResource(shared_from_this());
            m_components.push_back(component);
            return component;
        }
        
        template<typename ComponentType>
        std::shared_ptr<ComponentType> getComponent()
        {
            for (auto& component : m_components)
            {
                if (auto typed_component = std::dynamic_pointer_cast<ComponentType>(component))
                {
                    return typed_component;
                }
            }
            return nullptr;
        }
        
        template<typename ComponentType>
        void removeComponent()
        {
            m_components.erase(
                std::remove_if(m_components.begin(), m_components.end(),
                    [](const std::shared_ptr<Component>& component) {
                        return std::dynamic_pointer_cast<ComponentType>(component) != nullptr;
                    }),
                m_components.end()
            );
        }
        
        template<typename ComponentType>
        bool hasComponent() const
        {
            for (const auto& component : m_components)
            {
                if (std::dynamic_pointer_cast<ComponentType>(component))
                {
                    return true;
                }
            }
            return false;
        }
        
        // 获取所有组件
        const std::vector<std::shared_ptr<Component>>& getComponents() const { return m_components; }
        
        // 更新
        void tick(float delta_time);
        
        // 序列化支持
        void postLoadResource();
        
    private:
        size_t m_id = 0;
        std::string m_name;
        bool m_active = true;
        
        // ECS集成
        Entity m_ecs_entity = INVALID_ENTITY;
        
        // 组件列表
        std::vector<std::shared_ptr<Component>> m_components;
    };
    
} // namespace Piccolo
