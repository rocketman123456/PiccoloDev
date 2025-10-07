# GPU性能分析器集成指南

## 概述

本性能分析器已成功集成到Piccolo渲染系统中，提供了GPU时间戳查询功能，用于测量渲染管线的各个阶段的性能。

## 主要组件

### 1. GPUTimestampManager
- 管理Vulkan时间戳查询池
- 处理时间戳数据的收集和解析
- 支持多帧并发查询

### 2. GPUProfiler
- 高级性能分析接口
- 自动管理时间戳查询的生命周期
- 提供简单的push/pop API

### 3. RenderSystem集成
- 自动在每帧渲染时进行性能分析
- 提供访问性能分析器的接口
- 无需额外配置即可使用

## 使用方法

### 自动性能分析

性能分析器已自动集成到RenderSystem中：

```cpp
// 创建渲染系统
auto render_system = std::make_shared<RenderSystem>();
render_system->initialize();

// 渲染循环 - 性能分析会自动进行
for (int frame = 0; frame < 100; ++frame)
{
    render_system->tick(0.016f); // 60FPS
    
    // 获取性能数据
    auto profiler = render_system->getProfiler();
    if (profiler->hasValidData())
    {
        uint32_t count = profiler->getTimestampCount();
        const GPUTimestamp* timestamps = profiler->getTimestamps();
        
        for (uint32_t i = 0; i < count; ++i)
        {
            std::cout << timestamps[i].name << ": " 
                     << timestamps[i].elapsed_ms << "ms\n";
        }
    }
}
```

### 手动性能分析

如果需要测量特定的渲染操作：

```cpp
auto profiler = render_system->getProfiler();
VkCommandBuffer cmd = command_pool->getCommandBuffer(frame_index);

// 开始测量
profiler->beginTimestamp(cmd, frame_index, "My Custom Operation");
// ... 执行GPU操作 ...
profiler->endTimestamp(cmd, frame_index);
```

## 性能数据结构

```cpp
struct GPUTimestamp
{
    uint32_t start;           // 开始查询索引
    uint32_t end;             // 结束查询索引
    double elapsed_ms;        // 经过时间（毫秒）
    uint16_t parent_index;    // 父级索引
    uint16_t depth;           // 嵌套深度
    uint32_t color;           // 显示颜色
    uint32_t frame_index;     // 帧索引
    const char* name;         // 操作名称
};
```

## 配置选项

性能分析器在初始化时可以配置：

```cpp
// 在RenderSystem::initialize()中
m_profiler->initialize(m_device->getDevice(), 64, 3);
//                                    ^     ^
//                              每帧查询数  最大帧数
```

- **每帧查询数**: 每帧最多支持的时间戳查询数量
- **最大帧数**: 支持的最大并发帧数

## 注意事项

1. **性能开销**: 时间戳查询会有轻微的性能开销，建议在调试时使用
2. **查询限制**: 每帧的查询数量有限制，超出限制的查询会被忽略
3. **帧同步**: 性能数据在帧结束后才可用，需要等待GPU完成执行
4. **多线程**: 当前实现不是线程安全的，需要在主渲染线程中使用

## 扩展功能

### 添加更多测量点

可以在渲染管线的关键位置添加更多测量点：

```cpp
// 在GPUCommandPool::recordCommandBuffer()中添加
profiler->beginTimestamp(command_buffer, frame_index, "Geometry Pass");
// ... 几何渲染 ...
profiler->endTimestamp(command_buffer, frame_index);

profiler->beginTimestamp(command_buffer, frame_index, "Lighting Pass");
// ... 光照计算 ...
profiler->endTimestamp(command_buffer, frame_index);
```

### 可视化性能数据

可以结合ImGui等UI库来可视化性能数据：

```cpp
if (profiler->hasValidData())
{
    const GPUTimestamp* timestamps = profiler->getTimestamps();
    uint32_t count = profiler->getTimestampCount();
    
    // 绘制性能图表
    ImGui::Begin("GPU Profiler");
    for (uint32_t i = 0; i < count; ++i)
    {
        ImGui::Text("%s: %.3f ms", timestamps[i].name, timestamps[i].elapsed_ms);
    }
    ImGui::End();
}
```

## 故障排除

### 常见问题

1. **没有性能数据**: 确保在调用`hasValidData()`之前调用了`endFrame()`
2. **查询被忽略**: 检查是否超出了每帧查询数量限制
3. **时间戳为0**: 可能是GPU不支持时间戳查询或查询池创建失败

### 调试信息

启用日志系统可以查看详细的调试信息：

```cpp
// 在初始化时会输出：
// "GPU Profiler initialized with 64 queries per frame, 3 max frames"
// "GPU Timestamp Manager initialized with 64 queries per frame, 3 max frames"
```

## 未来改进

1. **CPU性能分析**: 添加CPU端的性能测量
2. **内存使用分析**: 监控GPU内存使用情况
3. **更丰富的可视化**: 提供更详细的性能图表
4. **配置文件支持**: 允许通过配置文件调整性能分析设置
