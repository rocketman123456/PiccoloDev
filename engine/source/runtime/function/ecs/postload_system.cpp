#include "postload_system.h"
#include "runtime/resource/res_type/components/lua_script.h"

#include <algorithm>
#include <iostream>

namespace Piccolo
{
    // PostLoadSystem 实现
    void PostLoadSystem::addTask(const PostLoadTask& task)
    {
        m_task_queue.push(task);
    }
    
    void PostLoadSystem::addTask(PostLoadTaskType type, Entity entity, std::function<void()> task, int priority, const std::string& description)
    {
        m_task_queue.push(PostLoadTask(type, entity, task, priority, description));
    }
    
    void PostLoadSystem::executeAllTasks()
    {
        while (!m_task_queue.empty())
        {
            PostLoadTask task = m_task_queue.top();
            m_task_queue.pop();
            
            try
            {
                task.task();
            }
            catch (const std::exception& e)
            {
                std::cerr << "PostLoad task execution failed: " << e.what() << std::endl;
                if (!task.description.empty())
                {
                    std::cerr << "Task description: " << task.description << std::endl;
                }
            }
        }
    }
    
    void PostLoadSystem::executeTasksOfType(PostLoadTaskType type)
    {
        std::vector<PostLoadTask> remaining_tasks;
        
        while (!m_task_queue.empty())
        {
            PostLoadTask task = m_task_queue.top();
            m_task_queue.pop();
            
            if (task.type == type)
            {
                try
                {
                    task.task();
                }
                catch (const std::exception& e)
                {
                    std::cerr << "PostLoad task execution failed: " << e.what() << std::endl;
                    if (!task.description.empty())
                    {
                        std::cerr << "Task description: " << task.description << std::endl;
                    }
                }
            }
            else
            {
                remaining_tasks.push_back(task);
            }
        }
        
        // 将剩余任务放回队列
        for (const auto& task : remaining_tasks)
        {
            m_task_queue.push(task);
        }
    }
    
    void PostLoadSystem::executeTasksForEntity(Entity entity)
    {
        std::vector<PostLoadTask> remaining_tasks;
        
        while (!m_task_queue.empty())
        {
            PostLoadTask task = m_task_queue.top();
            m_task_queue.pop();
            
            if (task.entity == entity)
            {
                try
                {
                    task.task();
                }
                catch (const std::exception& e)
                {
                    std::cerr << "PostLoad task execution failed: " << e.what() << std::endl;
                    if (!task.description.empty())
                    {
                        std::cerr << "Task description: " << task.description << std::endl;
                    }
                }
            }
            else
            {
                remaining_tasks.push_back(task);
            }
        }
        
        // 将剩余任务放回队列
        for (const auto& task : remaining_tasks)
        {
            m_task_queue.push(task);
        }
    }
    
    void PostLoadSystem::clearAllTasks()
    {
        while (!m_task_queue.empty())
        {
            m_task_queue.pop();
        }
    }
    
    size_t PostLoadSystem::getTaskCountOfType(PostLoadTaskType type) const
    {
        // 由于priority_queue不支持遍历，这里需要复制队列来计算
        auto temp_queue = m_task_queue;
        size_t count = 0;
        
        while (!temp_queue.empty())
        {
            if (temp_queue.top().type == type)
            {
                count++;
            }
            temp_queue.pop();
        }
        
        return count;
    }
    
    void PostLoadSystem::addComponentInitializationTasks(Registry& registry)
    {
        // 为所有实体添加组件初始化任务
        auto view = registry.view<NameComponent>();
        for (auto entity : view)
        {
            addTask(PostLoadTaskType::ComponentInitialization, entity, 
                [this, entity, &registry]() { executeComponentInitialization(entity, registry); },
                0, "Component initialization for entity");
        }
    }
    
    void PostLoadSystem::addDependencyResolutionTasks(Registry& registry)
    {
        // 为所有实体添加依赖解析任务
        auto view = registry.view<NameComponent>();
        for (auto entity : view)
        {
            addTask(PostLoadTaskType::ComponentDependency, entity,
                [this, entity, &registry]() { executeDependencyResolution(entity, registry); },
                1, "Dependency resolution for entity");
        }
    }
    
    void PostLoadSystem::addResourceLoadingTasks(Registry& registry)
    {
        // 为有资源组件的实体添加资源加载任务
        auto renderable_view = registry.view<RenderableComponent>();
        for (auto entity : renderable_view)
        {
            addTask(PostLoadTaskType::ResourceLoading, entity,
                [this, entity, &registry]() { executeResourceLoading(entity, registry); },
                2, "Resource loading for renderable entity");
        }
        
        auto script_view = registry.view<LuaScriptComponent>();
        for (auto entity : script_view)
        {
            addTask(PostLoadTaskType::ResourceLoading, entity,
                [this, entity, &registry]() { executeResourceLoading(entity, registry); },
                2, "Resource loading for script entity");
        }
    }
    
    void PostLoadSystem::executeComponentInitialization(Entity entity, Registry& registry)
    {
        // 初始化变换组件
        if (registry.all_of<TransformComponent>(entity))
        {
            auto& transform = registry.get<TransformComponent>(entity);
            transform.updateTransformMatrix();
        }
        
        // 初始化相机组件
        if (registry.all_of<CameraComponent>(entity))
        {
            auto& camera = registry.get<CameraComponent>(entity);
            // 这里可以添加相机初始化逻辑
        }
        
        // 初始化光源组件
        if (registry.all_of<LightComponent>(entity))
        {
            auto& light = registry.get<LightComponent>(entity);
            // 这里可以添加光源初始化逻辑
        }
        
        // 初始化Lua脚本组件
        if (registry.all_of<LuaScriptComponent>(entity))
        {
            auto& script = registry.get<LuaScriptComponent>(entity);
            if (script.script_resource)
            {
                script.script_resource->loadScriptContent();
            }
        }
    }
    
    void PostLoadSystem::executeDependencyResolution(Entity entity, Registry& registry)
    {
        // 解析组件依赖关系
        // 例如：相机组件可能依赖于变换组件
        if (registry.all_of<CameraComponent>(entity) && registry.all_of<TransformComponent>(entity))
        {
            auto& transform = registry.get<TransformComponent>(entity);
            auto& camera = registry.get<CameraComponent>(entity);
            
            // 更新相机的视图矩阵
            // 这里可以添加相机依赖解析逻辑
        }
        
        // 解析渲染组件依赖
        if (registry.all_of<RenderableComponent>(entity) && registry.all_of<TransformComponent>(entity))
        {
            auto& transform = registry.get<TransformComponent>(entity);
            auto& renderable = registry.get<RenderableComponent>(entity);
            
            // 更新渲染组件的变换矩阵
            // 这里可以添加渲染依赖解析逻辑
        }
    }
    
    void PostLoadSystem::executeResourceLoading(Entity entity, Registry& registry)
    {
        // 加载渲染资源
        if (registry.all_of<RenderableComponent>(entity))
        {
            auto& renderable = registry.get<RenderableComponent>(entity);
            
            // 加载网格资源
            if (!renderable.mesh_url.empty())
            {
                // 这里应该调用资源管理器加载网格
                // ResourceManager::getInstance().loadMesh(renderable.mesh_url);
            }
            
            // 加载材质资源
            if (!renderable.material_url.empty())
            {
                // 这里应该调用资源管理器加载材质
                // ResourceManager::getInstance().loadMaterial(renderable.material_url);
            }
        }
        
        // 加载脚本资源
        if (registry.all_of<LuaScriptComponent>(entity))
        {
            auto& script = registry.get<LuaScriptComponent>(entity);
            
            if (!script.script_url.empty() && !script.script_resource)
            {
                // 这里应该创建并加载脚本资源
                // script.script_resource = ResourceManager::getInstance().loadLuaScript(script.script_url);
            }
        }
    }
    
    // PostLoadManager 实现
    void PostLoadManager::registerPostLoadSystem(std::shared_ptr<PostLoadSystem> system)
    {
        m_postload_system = system;
    }
    
    std::shared_ptr<PostLoadSystem> PostLoadManager::getPostLoadSystem()
    {
        if (!m_postload_system)
        {
            m_postload_system = std::make_shared<PostLoadSystem>();
        }
        return m_postload_system;
    }
    
    void PostLoadManager::executeAllPostLoadTasks()
    {
        if (m_postload_system)
        {
            m_postload_system->executeAllTasks();
        }
    }
    
    void PostLoadManager::addPostLoadTasksForRegistry(Registry& registry)
    {
        if (m_postload_system)
        {
            // 按顺序添加PostLoad任务
            m_postload_system->addComponentInitializationTasks(registry);
            m_postload_system->addDependencyResolutionTasks(registry);
            m_postload_system->addResourceLoadingTasks(registry);
        }
    }
    
    void PostLoadManager::clearAllPostLoadTasks()
    {
        if (m_postload_system)
        {
            m_postload_system->clearAllTasks();
        }
    }
    
} // namespace Piccolo
