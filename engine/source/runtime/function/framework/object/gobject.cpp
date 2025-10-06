#include "gobject.h"
#include "runtime/function/framework/component/component.h"

namespace Piccolo
{
    // GObject 实现
    GObject::GObject(size_t id, const std::string& name) 
        : m_id(id), m_name(name)
    {
    }
    
    void GObject::tick(float delta_time)
    {
        if (!m_active)
            return;
        
        // 更新所有组件
        for (auto& component : m_components)
        {
            if (component)
            {
                component->tick(delta_time);
            }
        }
    }
    
    void GObject::postLoadResource()
    {
        // 调用所有组件的postLoadResource方法
        for (auto& component : m_components)
        {
            if (component)
            {
                component->postLoadResource(shared_from_this());
            }
        }
    }
    
} // namespace Piccolo
