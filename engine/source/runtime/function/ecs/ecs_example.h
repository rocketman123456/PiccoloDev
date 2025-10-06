#pragma once
#include "runtime/function/ecs/components.h"
#include "runtime/function/ecs/ecs_serializer.h"
#include "runtime/function/ecs/entt_coordinator.h"
#include "runtime/function/ecs/postload_system.h"
#include "runtime/function/ecs/systems.h"
#include "runtime/function/ecs/types.h"

#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/world/world.h"
#include "runtime/resource/res_type/common/level.h"
#include "runtime/resource/res_type/common/world.h"

#include <string>
#include <vector>

namespace Piccolo
{
    // ECS使用示例
    class ECSExample
    {
    public:
        ECSExample()  = default;
        ~ECSExample() = default;

        // 基础ECS使用示例
        void basicECSExample();

        // 世界和关卡示例
        void worldAndLevelExample();

        // 资源序列化示例
        void resourceSerializationExample();

        // PostLoad系统示例
        void postLoadSystemExample();

        // 创建示例场景
        void createSampleScene();

    private:
        // 辅助函数
        void printEntityInfo(Entity entity, Registry& registry);
        void printComponentInfo(Entity entity, Registry& registry);
    };

    // 示例实现
    void ECSExample::basicECSExample()
    {
        // 获取ECS协调器
        auto& coordinator = ecs::getCoordinator();
        auto& registry    = coordinator.getRegistry();

        // 创建实体
        Entity player = coordinator.createEntity();
        Entity enemy  = coordinator.createEntity();
        Entity camera = coordinator.createEntity();

        // 添加组件
        coordinator.addComponent<NameComponent>(player, "Player", true);
        coordinator.addComponent<TransformComponent>(player, Vector3(0.0f, 0.0f, 0.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        coordinator.addComponent<RenderableComponent>(player, "player_mesh.obj", "player_material");

        coordinator.addComponent<NameComponent>(enemy, "Enemy", true);
        coordinator.addComponent<TransformComponent>(enemy, Vector3(10.0f, 0.0f, 0.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        coordinator.addComponent<RenderableComponent>(enemy, "enemy_mesh.obj", "enemy_material");

        coordinator.addComponent<NameComponent>(camera, "MainCamera", true);
        coordinator.addComponent<TransformComponent>(camera, Vector3(0.0f, 0.0f, 10.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        coordinator.addComponent<CameraComponent>(camera, 45.0f, 0.1f, 1000.0f, true);

        // 查询实体
        auto renderable_view = registry.view<TransformComponent, RenderableComponent>();
        for (auto entity : renderable_view)
        {
            auto& transform  = renderable_view.get<TransformComponent>(entity);
            auto& renderable = renderable_view.get<RenderableComponent>(entity);

            // 处理可渲染实体
            if (renderable.visible)
            {
                // 更新变换矩阵
                transform.updateTransformMatrix();
                // 提交渲染命令等...
            }
        }

        // 清理
        coordinator.destroyEntity(player);
        coordinator.destroyEntity(enemy);
        coordinator.destroyEntity(camera);
    }

    void ECSExample::worldAndLevelExample()
    {
        // 创建世界
        auto& world_manager = getWorldManager();
        auto  world         = world_manager.createWorld("TestWorld");

        // 创建关卡
        auto& level_manager = getLevelManager();
        auto  level         = level_manager.createLevel("TestLevel");

        // 在世界中创建实体
        Entity world_entity = world->createWorldEntity("WorldSettings");
        world->addWorldComponent<WorldComponent>(world_entity, "TestWorld", "TestLevel");

        // 在关卡中创建实体
        Entity player = level->createEntity("Player");
        level->addComponent<TransformComponent>(player, Vector3(0.0f, 0.0f, 0.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        level->addComponent<RenderableComponent>(player, "player_mesh.obj", "player_material");

        // 创建对应的GObject
        auto   gobject       = level->createGObject("Player");
        Entity player_entity = level->getEntityByObjectID(gobject->getID());

        // 更新系统
        world->update(0.016f); // 60 FPS
        level->update(0.016f);

        // 清理
        world_manager.destroyWorld("TestWorld");
        level_manager.destroyLevel("TestLevel");
    }

    void ECSExample::resourceSerializationExample()
    {
        // 创建关卡资源
        LevelRes level_res;
        level_res.m_objects.push_back(ObjectInstanceRes());
        level_res.m_objects[0].m_name   = "TestObject";
        level_res.m_objects[0].m_active = true;

        // 从资源创建关卡
        auto& level_manager = getLevelManager();
        auto  level         = level_manager.createLevel("SerializationTest");

        // 加载资源
        level->loadFromResource(level_res);

        // 保存资源
        LevelRes saved_res = level->saveToResource();

        // 清理
        level_manager.destroyLevel("SerializationTest");
    }

    void ECSExample::postLoadSystemExample()
    {
        auto& coordinator = ecs::getCoordinator();
        auto& registry    = coordinator.getRegistry();

        // 创建实体
        Entity entity = coordinator.createEntity();
        coordinator.addComponent<NameComponent>(entity, "PostLoadTest");
        coordinator.addComponent<TransformComponent>(entity);
        coordinator.addComponent<RenderableComponent>(entity, "test_mesh.obj", "test_material");

        // 添加PostLoad任务
        auto& postload_manager = getPostLoadManager();
        postload_manager.addPostLoadTasksForRegistry(registry);

        // 执行PostLoad任务
        postload_manager.executeAllPostLoadTasks();

        // 清理
        coordinator.destroyEntity(entity);
    }

    void ECSExample::createSampleScene()
    {
        // 创建世界
        auto& world_manager = getWorldManager();
        auto  world         = world_manager.createWorld("SampleWorld");

        // 创建关卡
        auto& level_manager = getLevelManager();
        auto  level         = level_manager.createLevel("SampleLevel");

        // 创建玩家
        Entity player = level->createEntity("Player");
        level->addComponent<TransformComponent>(player, Vector3(0.0f, 0.0f, 0.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        level->addComponent<RenderableComponent>(player, "player_mesh.obj", "player_material");
        level->addComponent<LuaScriptComponent>(player, "player_script.lua");

        // 创建敌人
        for (int i = 0; i < 5; ++i)
        {
            Entity enemy = level->createEntity("Enemy_" + std::to_string(i));
            level->addComponent<TransformComponent>(enemy, Vector3(i * 5.0f, 0.0f, 0.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
            level->addComponent<RenderableComponent>(enemy, "enemy_mesh.obj", "enemy_material");
        }

        // 创建相机
        Entity camera = level->createEntity("MainCamera");
        level->addComponent<TransformComponent>(camera, Vector3(0.0f, 0.0f, 10.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        level->addComponent<CameraComponent>(camera, 45.0f, 0.1f, 1000.0f, true);

        // 创建光源
        Entity light = level->createEntity("MainLight");
        level->addComponent<TransformComponent>(light, Vector3(0.0f, 10.0f, 0.0f), Quaternion::IDENTITY, Vector3(1.0f, 1.0f, 1.0f));
        level->addComponent<LightComponent>(light, LightComponent::LightType::Directional, Vector3(1.0f, 1.0f, 1.0f), 1.0f);

        // 执行PostLoad任务
        level->executePostLoadTasks();

        // 模拟游戏循环
        for (int frame = 0; frame < 100; ++frame)
        {
            world->update(0.016f);
            level->update(0.016f);
        }

        // 清理
        world_manager.destroyWorld("SampleWorld");
        level_manager.destroyLevel("SampleLevel");
    }

    void ECSExample::printEntityInfo(Entity entity, Registry& registry)
    {
        if (registry.all_of<NameComponent>(entity))
        {
            const auto& name = registry.get<NameComponent>(entity);
            // 打印实体信息
        }
    }

    void ECSExample::printComponentInfo(Entity entity, Registry& registry)
    {
        // 打印组件信息
        if (registry.all_of<TransformComponent>(entity))
        {
            const auto& transform = registry.get<TransformComponent>(entity);
            // 打印变换信息
        }

        if (registry.all_of<RenderableComponent>(entity))
        {
            const auto& renderable = registry.get<RenderableComponent>(entity);
            // 打印渲染信息
        }
    }

} // namespace Piccolo