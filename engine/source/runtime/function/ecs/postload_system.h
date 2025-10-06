#pragma once
#include "types.h"
#include "components.h"
#include "entt_coordinator.h"

#include <vector>
#include <queue>
#include <functional>
#include <unordered_map>
#include <memory>

namespace Piccolo
{
    // PostLoad任务类型
    enum class PostLoadTaskType
    {
        ComponentInitialization,    // 组件初始化
        ComponentDependency,        // 组件依赖解析
        ResourceLoading,            // 资源加载
        SystemInitialization,       // 系统初始化
        Custom                      // 自定义任务
    };
    
    // PostLoad任务
    struct PostLoadTask
    {
        PostLoadTaskType type;
        Entity entity;
        std::function<void()> task;
        int priority; // 优先级，数字越小优先级越高
        std::string description;
        
        PostLoadTask(PostLoadTaskType t, Entity e, std::function<void()> func, int p = 0, const std::string& desc = "")
            : type(t), entity(e), task(func), priority(p), description(desc) {}
    };
    
    // PostLoad任务比较器（用于优先级队列）
    struct PostLoadTaskComparator
    {
        bool operator()(const PostLoadTask& a, const PostLoadTask& b) const
        {
            return a.priority > b.priority; // 优先级队列是最大堆，所以这里用>来让优先级小的先执行
        }
    };
    
    // PostLoad系统
    class PostLoadSystem
    {
    public:
        PostLoadSystem() = default;
        ~PostLoadSystem() = default;
        
        // 添加PostLoad任务
        void addTask(const PostLoadTask& task);
        void addTask(PostLoadTaskType type, Entity entity, std::function<void()> task, int priority = 0, const std::string& description = "");
        
        // 执行所有PostLoad任务
        void executeAllTasks();
        
        // 执行指定类型的任务
        void executeTasksOfType(PostLoadTaskType type);
        
        // 执行指定实体的任务
        void executeTasksForEntity(Entity entity);
        
        // 清空所有任务
        void clearAllTasks();
        
        // 获取任务数量
        size_t getTaskCount() const { return m_task_queue.size(); }
        size_t getTaskCountOfType(PostLoadTaskType type) const;
        
        // 检查是否有待执行的任务
        bool hasPendingTasks() const { return !m_task_queue.empty(); }
        
        // 批量添加组件初始化任务
        void addComponentInitializationTasks(Registry& registry);
        
        // 批量添加依赖解析任务
        void addDependencyResolutionTasks(Registry& registry);
        
        // 批量添加资源加载任务
        void addResourceLoadingTasks(Registry& registry);
        
    private:
        std::priority_queue<PostLoadTask, std::vector<PostLoadTask>, PostLoadTaskComparator> m_task_queue;
        
        // 辅助函数
        void executeComponentInitialization(Entity entity, Registry& registry);
        void executeDependencyResolution(Entity entity, Registry& registry);
        void executeResourceLoading(Entity entity, Registry& registry);
    };
    
    // PostLoad管理器
    class PostLoadManager
    {
    public:
        static PostLoadManager& getInstance()
        {
            static PostLoadManager instance;
            return instance;
        }
        
        // 禁用拷贝构造和赋值
        PostLoadManager(const PostLoadManager&) = delete;
        PostLoadManager& operator=(const PostLoadManager&) = delete;
        
        // 注册PostLoad系统
        void registerPostLoadSystem(std::shared_ptr<PostLoadSystem> system);
        
        // 获取PostLoad系统
        std::shared_ptr<PostLoadSystem> getPostLoadSystem();
        
        // 执行所有PostLoad任务
        void executeAllPostLoadTasks();
        
        // 为指定注册表添加PostLoad任务
        void addPostLoadTasksForRegistry(Registry& registry);
        
        // 清空所有PostLoad任务
        void clearAllPostLoadTasks();
        
    private:
        PostLoadManager() = default;
        ~PostLoadManager() = default;
        
        std::shared_ptr<PostLoadSystem> m_postload_system;
    };
    
    // 便捷的全局访问函数
    inline PostLoadManager& getPostLoadManager()
    {
        return PostLoadManager::getInstance();
    }
    
    // PostLoad任务构建器
    class PostLoadTaskBuilder
    {
    public:
        PostLoadTaskBuilder& setType(PostLoadTaskType type) { m_type = type; return *this; }
        PostLoadTaskBuilder& setEntity(Entity entity) { m_entity = entity; return *this; }
        PostLoadTaskBuilder& setTask(std::function<void()> task) { m_task = task; return *this; }
        PostLoadTaskBuilder& setPriority(int priority) { m_priority = priority; return *this; }
        PostLoadTaskBuilder& setDescription(const std::string& description) { m_description = description; return *this; }
        
        PostLoadTask build() const
        {
            return PostLoadTask(m_type, m_entity, m_task, m_priority, m_description);
        }
        
        void submit()
        {
            getPostLoadManager().getPostLoadSystem()->addTask(build());
        }
        
    private:
        PostLoadTaskType m_type = PostLoadTaskType::Custom;
        Entity m_entity = INVALID_ENTITY;
        std::function<void()> m_task;
        int m_priority = 0;
        std::string m_description;
    };
    
    // 便捷的PostLoad任务创建宏
    #define POSTLOAD_TASK(type, entity, task) \
        PostLoadTaskBuilder().setType(type).setEntity(entity).setTask(task)
    
    #define POSTLOAD_TASK_WITH_PRIORITY(type, entity, task, priority) \
        PostLoadTaskBuilder().setType(type).setEntity(entity).setTask(task).setPriority(priority)
    
} // namespace Piccolo
