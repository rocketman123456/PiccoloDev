#pragma once
#include "runtime/function/ecs/components.h"
#include "runtime/function/ecs/types.h"

#include <memory>
#include <unordered_map>

namespace Piccolo
{
    // ECS协调器 - 全局单例
    class EnTTCoordinator
    {
    public:
        static EnTTCoordinator& getInstance()
        {
            static EnTTCoordinator instance;
            return instance;
        }

        // 禁用拷贝构造和赋值
        EnTTCoordinator(const EnTTCoordinator&)            = delete;
        EnTTCoordinator& operator=(const EnTTCoordinator&) = delete;

        // 获取注册表
        Registry&       getRegistry() { return m_registry; }
        const Registry& getRegistry() const { return m_registry; }

        // 实体管理
        Entity createEntity() { return m_registry.create(); }

        void destroyEntity(Entity entity) { m_registry.destroy(entity); }

        bool isValid(Entity entity) const { return m_registry.valid(entity); }

        // 组件管理
        template<typename ComponentType, typename... Args>
        ComponentType& addComponent(Entity entity, Args&&... args)
        {
            return m_registry.emplace<ComponentType>(entity, std::forward<Args>(args)...);
        }

        template<typename ComponentType>
        void removeComponent(Entity entity)
        {
            m_registry.remove<ComponentType>(entity);
        }

        template<typename ComponentType>
        ComponentType& getComponent(Entity entity)
        {
            return m_registry.get<ComponentType>(entity);
        }

        template<typename ComponentType>
        const ComponentType& getComponent(Entity entity) const
        {
            return m_registry.get<ComponentType>(entity);
        }

        template<typename ComponentType>
        bool hasComponent(Entity entity) const
        {
            return m_registry.all_of<ComponentType>(entity);
        }

        template<typename ComponentType>
        ComponentType* tryGetComponent(Entity entity)
        {
            return m_registry.try_get<ComponentType>(entity);
        }

        template<typename ComponentType>
        const ComponentType* tryGetComponent(Entity entity) const
        {
            return m_registry.try_get<ComponentType>(entity);
        }

        // 实体查询
        template<typename... ComponentTypes>
        auto view()
        {
            return m_registry.view<ComponentTypes...>();
        }

        template<typename... ComponentTypes>
        auto view() const
        {
            return m_registry.view<ComponentTypes...>();
        }

        // 事件系统
        EventDispatcher&       getEventDispatcher() { return m_event_dispatcher; }
        const EventDispatcher& getEventDispatcher() const { return m_event_dispatcher; }

        // 清理所有实体
        void clear() { m_registry.clear(); }

        // 获取实体数量
        size_t getEntityCount() const
        {
            // TODO: 实现正确的实体计数
            return 0;
        }

    private:
        EnTTCoordinator()  = default;
        ~EnTTCoordinator() = default;

        Registry        m_registry;
        EventDispatcher m_event_dispatcher;
    };

    namespace ecs
    {
        // 便捷的全局访问函数
        inline EnTTCoordinator& getCoordinator() { return EnTTCoordinator::getInstance(); }

        inline Registry& getRegistry() { return getCoordinator().getRegistry(); }
    } // namespace ecs

} // namespace Piccolo