# 渲染系统重构说明

## 概述

本次重构旨在简化渲染代码结构，添加更多GPU/渲染工具，并使创建管道和渲染通道更加紧凑。重构保持了向后兼容性，同时提供了更现代和易用的API。

## 主要改进

### 1. 管道构建器 (GPUPipelineBuilder)

**位置**: `utils/gpu_pipeline_builder.h` 和 `utils/gpu_pipeline_builder.cpp`

**功能**:
- 流式API设计，支持链式调用
- 预定义配置工厂，快速创建常用管道
- 完整的Vulkan管道状态配置支持
- 自动资源管理和错误处理

**使用示例**:
```cpp
// 使用构建器创建管道
GPUPipelineBuilder builder(device);
auto pipeline = builder
    .setName("My Pipeline")
    .addVertexShader("shader.vert")
    .addFragmentShader("shader.frag")
    .enableDepthTest(true)
    .setCullMode(VK_CULL_MODE_BACK_BIT)
    .setRenderPass(render_pass)
    .buildGraphicsPipeline();

// 使用预定义配置
auto config = GPUPipelineConfigFactory::createDepthTestPipeline();
auto pipeline = std::make_shared<GPUPipeline>(device, config, render_pass);
```

### 2. 渲染通道构建器 (GPURenderPassBuilder)

**位置**: `utils/gpu_render_pass_builder.h` 和 `utils/gpu_render_pass_builder.cpp`

**功能**:
- 简化的渲染通道创建流程
- 支持复杂的多附件和多子通道配置
- 预定义渲染通道模板
- 自动处理子通道依赖关系

**使用示例**:
```cpp
// 使用构建器创建渲染通道
GPURenderPassBuilder builder(device);
auto render_pass = builder
    .setName("Deferred Pass")
    .addColorAttachment(color_attachment_config)
    .addDepthAttachment(depth_attachment_config)
    .addSubpass("G-Buffer Subpass")
    .addDependency(VK_SUBPASS_EXTERNAL, 0)
    .build();

// 使用预定义配置
auto config = GPURenderPassConfigFactory::createDepthColorPass(color_format, depth_format);
auto render_pass = std::make_shared<GPURenderPass>(device, config);
```

### 3. 渲染工具类 (GPURenderUtils)

**位置**: `utils/gpu_render_utils.h` 和 `utils/gpu_render_utils.cpp`

**功能**:
- **GPURenderResourceManager**: 统一管理渲染资源
- **GPURenderCommandBuilder**: 流式命令记录API
- **GPURenderStateManager**: 渲染状态管理
- **GPURenderUtils**: 静态工具函数集合

**使用示例**:
```cpp
// 资源管理
GPURenderResourceManager resource_manager(device);
auto pipeline = resource_manager.createPipeline(config, render_pass);
auto framebuffer = resource_manager.createFramebuffer("Main", render_pass, attachments, width, height);

// 命令记录
GPURenderCommandBuilder cmd(command_buffer);
cmd.beginRenderPass(render_pass, framebuffer, render_area, clear_values)
   .setViewport(viewport)
   .setScissor(scissor)
   .bindPipeline(pipeline)
   .draw(3, 1, 0, 0)
   .endRenderPass();

// 工具函数
auto viewport = GPURenderUtils::createViewport(0, 0, 1920, 1080);
auto clear_color = GPURenderUtils::createClearColor(0.0f, 0.0f, 0.0f, 1.0f);
```

## 重构的类

### GPUPipeline
- 添加了新的构造函数，支持GPUPipelineBuilderConfig
- 保持向后兼容的旧构造函数
- 添加了静态工厂方法

### GPURenderPass
- 添加了新的构造函数，支持GPURenderPassConfig
- 保持向后兼容的旧构造函数
- 添加了静态工厂方法

### RenderSystem
- 集成了新的工具类
- 简化了初始化流程
- 使用新的工厂方法创建默认资源

## 向后兼容性

所有现有的代码将继续工作，因为：
1. 保留了原有的构造函数和API
2. 新的功能通过额外的构造函数和静态方法提供
3. 没有破坏现有的接口

## 性能优化

1. **资源复用**: GPURenderResourceManager提供资源缓存和复用
2. **减少Vulkan调用**: 构建器模式减少了重复的Vulkan API调用
3. **内存管理**: 自动管理Vulkan对象的生命周期
4. **错误处理**: 统一的错误处理和日志记录

## 使用建议

1. **新项目**: 直接使用新的构建器API和工具类
2. **现有项目**: 可以逐步迁移到新API，或继续使用旧API
3. **复杂渲染**: 使用GPURenderResourceManager管理多个管道和渲染通道
4. **性能关键**: 使用预定义的配置工厂避免重复配置

## 文件结构

```
engine/source/runtime/function/render/
├── utils/
│   ├── gpu_pipeline_builder.h/cpp      # 管道构建器
│   ├── gpu_render_pass_builder.h/cpp   # 渲染通道构建器
│   ├── gpu_render_utils.h/cpp          # 渲染工具类
│   └── gpu_utils.h/cpp                 # 基础工具函数
├── examples/
│   └── render_example.cpp              # 使用示例
├── gpu_pipeline.h/cpp                  # 重构后的管道类
├── gpu_render_pass.h/cpp               # 重构后的渲染通道类
├── render_system.h/cpp                 # 重构后的渲染系统
└── README_REFACTOR.md                  # 本文档
```

## 下一步计划

1. 添加更多预定义配置模板
2. 实现渲染图(Render Graph)支持
3. 添加GPU内存管理工具
4. 实现多线程渲染支持
5. 添加性能分析和调试工具
