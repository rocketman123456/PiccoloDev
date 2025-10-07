# EnTT ECS 集成

本文档描述了 Piccolo 引擎中 EnTT ECS 库的集成和使用方法。

## 概述

Piccolo 引擎现在使用 EnTT 作为其 ECS (Entity-Component-System) 库，提供了高性能的实体组件系统。EnTT 是一个现代的 C++ ECS 库，具有出色的性能和易用性。

## 主要组件

### 1. 类型定义 (`types.h`)
- `Entity`: EnTT 实体类型
- `Registry`: EnTT 注册表类型
- 事件系统类型定义

### 2. 组件 (`components.h`)
定义了常用的游戏组件：
- `TransformComponent`: 位置、旋转、缩放
- `NameComponent`: 实体名称和激活状态
- `ObjectIDComponent`: 与现有 GObject 系统的兼容性
- `TagComponent`: 实体标签
- `LevelComponent`: 关卡引用
- `WorldComponent`: 世界引用
- `RenderableComponent`: 可渲染对象
- `CameraComponent`: 相机
- `LightComponent`: 光源

### 3. 协调器 (`entt_coordinator.h`)
提供了全局的 ECS 协调器，用于管理实体和组件。

### 4. 系统 (`systems.h`)
示例系统实现：
- `TransformSystem`: 变换更新
- `RenderSystem`: 渲染处理
- `CameraSystem`: 相机管理
- `LightSystem`: 光照管理
- `NameSystem`: 名称和标签管理

## 使用方法

### 在 Level 中使用 ECS

```cpp
#include "runtime/function/ecs/components.h"
#include "runtime/function/ecs/systems.h"

// 创建实体
Entity entity = level.createEntity("MyEntity");

// 添加组件
level.addComponent<TransformComponent>(entity, 
    Vec3(0.0f, 0.0f, 0.0f),  // position
    Vec3(0.0f, 0.0f, 0.0f),  // rotation
    Vec3(1.0f, 1.0f, 1.0f)   // scale
);

level.addComponent<RenderableComponent>(entity, "mesh.obj", "material");

// 检查组件
if (level.hasComponent<TransformComponent>(entity))
{
    auto& transform = level.getComponent<TransformComponent>(entity);
    // 使用变换组件
}

// 查询实体
auto view = level.view<TransformComponent, RenderableComponent>();
for (auto entity : view)
{
    auto& transform = view.get<TransformComponent>(entity);
    auto& renderable = view.get<RenderableComponent>(entity);
    // 处理实体
}
```

### 在 WorldManager 中使用 ECS

```cpp
// 创建世界级实体
Entity world_entity = world_manager.createWorldEntity("WorldSettings");

// 添加世界级组件
world_manager.addWorldComponent<NameComponent>(world_entity, "WorldSettings");

// 查询世界级实体
auto world_view = world_manager.viewWorld<NameComponent>();
for (auto entity : world_view)
{
    auto& name = world_view.get<NameComponent>(entity);
    // 处理世界级实体
}
```

### 使用系统

```cpp
// 创建系统实例
TransformSystem transform_system;
RenderSystem render_system;

// 在更新循环中使用
void update(float delta_time)
{
    transform_system.update(level.getRegistry(), delta_time);
    render_system.update(level.getRegistry(), delta_time);
}
```

## 与现有系统的兼容性

新的 ECS 系统与现有的 GObject 系统保持兼容：

- `GObject` 类现在包含 `m_ecs_entity` 成员
- `Level` 类同时支持 GObject 和 ECS 实体
- 提供了 `getEntityByObjectID()` 和 `getObjectIDByEntity()` 方法进行转换

## 性能优势

EnTT 提供了以下性能优势：

1. **内存局部性**: 组件数据按类型存储，提高缓存效率
2. **快速查询**: 使用位掩码进行快速的实体查询
3. **零开销抽象**: 编译时优化，运行时开销最小
4. **并行友好**: 支持多线程处理

## 示例场景

参考 `ecs_example.h` 文件中的 `ECSExample::createSampleScene()` 方法，了解如何创建完整的游戏场景。

## 注意事项

1. 确保在使用 EnTT 功能之前正确初始化
2. 组件应该是 POD (Plain Old Data) 类型以获得最佳性能
3. 避免在组件中存储指针，使用实体引用代替
4. 合理使用实体查询，避免过度查询

## 扩展

要添加新的组件类型：

1. 在 `components.h` 中定义组件结构
2. 在相应的系统中添加处理逻辑
3. 更新序列化代码（如果需要）

要添加新的系统：

1. 在 `systems.h` 中定义系统类
2. 实现 `update()` 方法
3. 在游戏循环中调用系统更新