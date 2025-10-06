#pragma once
#include "simple_ecs.h"
#include "runtime/function/framework/world/simple_world.h"
#include "runtime/function/framework/level/simple_level.h"
#include "runtime/resource/res_type/common/world.h"
#include "runtime/resource/res_type/common/level.h"

#include <string>
#include <vector>

namespace Piccolo
{
    // 简化ECS使用示例
    class SimpleECSExample
    {
    public:
        SimpleECSExample() = default;
        ~SimpleECSExample() = default;
        
        // 基础ECS使用示例
        void basicECSExample();
        
        // 世界和关卡示例
        void worldAndLevelExample();
        
        // 资源序列化示例
        void resourceSerializationExample();
        
        // 创建示例场景
        void createSampleScene();
        
    private:
        // 辅助函数
        void printEntityInfo(Entity* entity);
        void printComponentInfo(Entity* entity);
    };
    
    // 示例实现
    void SimpleECSExample::basicECSExample()
    {
        // 创建实体管理器
        EntityManager entity_manager;
        
        // 创建实体
        Entity* player = entity_manager.createEntity();
        Entity* enemy = entity_manager.createEntity();
        Entity* camera = entity_manager.createEntity();
        
        // 添加组件
        player->addComponent<NameComponent>("Player", true);
        player->addComponent<TransformComponent>(0.0f, 0.0f, 0.0f);
        player->addComponent<RenderableComponent>("player_mesh.obj", "player_material");
        
        enemy->addComponent<NameComponent>("Enemy", true);
        enemy->addComponent<TransformComponent>(10.0f, 0.0f, 0.0f);
        enemy->addComponent<RenderableComponent>("enemy_mesh.obj", "enemy_material");
        
        camera->addComponent<NameComponent>("MainCamera", true);
        camera->addComponent<TransformComponent>(0.0f, 0.0f, 10.0f);
        camera->addComponent<CameraComponent>(45.0f, 0.1f, 1000.0f, true);
        
        // 创建系统管理器
        SystemManager system_manager;
        system_manager.addSystem(std::make_unique<TransformSystem>());
        system_manager.addSystem(std::make_unique<RenderSystem>());
        
        // 初始化系统
        system_manager.initialize(entity_manager);
        
        // 模拟游戏循环
        for (int frame = 0; frame < 100; ++frame)
        {
            system_manager.update(entity_manager, 0.016f); // 60 FPS
        }
        
        // 执行PostLoad
        entity_manager.postLoad();
        
        // 清理
        system_manager.shutdown(entity_manager);
    }
    
    void SimpleECSExample::worldAndLevelExample()
    {
        // 创建世界
        auto& world_manager = getSimpleWorldManager();
        auto world = world_manager.createWorld("TestWorld");
        
        // 创建关卡
        auto& level_manager = getSimpleLevelManager();
        auto level = level_manager.createLevel("TestLevel");
        
        // 在世界中创建实体
        Entity* world_entity = world->createWorldEntity("WorldSettings");
        if (world_entity)
        {
            world_entity->addComponent<NameComponent>("WorldSettings");
        }
        
        // 在关卡中创建实体
        Entity* player = level->createEntity("Player");
        if (player)
        {
            player->addComponent<TransformComponent>(0.0f, 0.0f, 0.0f);
            player->addComponent<RenderableComponent>("player_mesh.obj", "player_material");
        }
        
        // 更新系统
        world->update(0.016f); // 60 FPS
        level->update(0.016f);
        
        // 清理
        world_manager.destroyWorld("TestWorld");
        level_manager.destroyLevel("TestLevel");
    }
    
    void SimpleECSExample::resourceSerializationExample()
    {
        // 创建关卡资源
        LevelRes level_res;
        ObjectInstanceRes object_instance;
        object_instance.m_name = "TestObject";
        object_instance.m_active = true;
        level_res.m_objects.push_back(object_instance);
        
        // 从资源创建关卡
        auto& level_manager = getSimpleLevelManager();
        auto level = level_manager.createLevel("SerializationTest");
        
        // 加载资源
        level->loadFromResource(level_res);
        
        // 保存资源
        LevelRes saved_res = level->saveToResource();
        
        // 清理
        level_manager.destroyLevel("SerializationTest");
    }
    
    void SimpleECSExample::createSampleScene()
    {
        // 创建世界
        auto& world_manager = getSimpleWorldManager();
        auto world = world_manager.createWorld("SampleWorld");
        
        // 创建关卡
        auto& level_manager = getSimpleLevelManager();
        auto level = level_manager.createLevel("SampleLevel");
        
        // 创建玩家
        Entity* player = level->createEntity("Player");
        if (player)
        {
            player->addComponent<TransformComponent>(0.0f, 0.0f, 0.0f);
            player->addComponent<RenderableComponent>("player_mesh.obj", "player_material");
        }
        
        // 创建敌人
        for (int i = 0; i < 5; ++i)
        {
            Entity* enemy = level->createEntity("Enemy_" + std::to_string(i));
            if (enemy)
            {
                enemy->addComponent<TransformComponent>(i * 5.0f, 0.0f, 0.0f);
                enemy->addComponent<RenderableComponent>("enemy_mesh.obj", "enemy_material");
            }
        }
        
        // 创建相机
        Entity* camera = level->createEntity("MainCamera");
        if (camera)
        {
            camera->addComponent<TransformComponent>(0.0f, 0.0f, 10.0f);
            camera->addComponent<CameraComponent>(45.0f, 0.1f, 1000.0f, true);
        }
        
        // 创建光源
        Entity* light = level->createEntity("MainLight");
        if (light)
        {
            light->addComponent<TransformComponent>(0.0f, 10.0f, 0.0f);
            light->addComponent<LightComponent>(LightComponent::LightType::Directional, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        
        // 执行PostLoad
        level->executePostLoad();
        
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
    
    void SimpleECSExample::printEntityInfo(Entity* entity)
    {
        if (!entity)
            return;
        
        // 打印实体信息
        if (auto* name_comp = entity->getComponent<NameComponent>())
        {
            // 这里可以打印名称信息
        }
    }
    
    void SimpleECSExample::printComponentInfo(Entity* entity)
    {
        if (!entity)
            return;
        
        // 打印组件信息
        if (auto* transform = entity->getComponent<TransformComponent>())
        {
            // 这里可以打印变换信息
        }
        
        if (auto* renderable = entity->getComponent<RenderableComponent>())
        {
            // 这里可以打印渲染信息
        }
    }
    
} // namespace Piccolo
