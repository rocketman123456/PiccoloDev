# GPU缓冲区工具函数使用指南

本文档介绍了Piccolo引擎中新增的GPU缓冲区工具函数，这些函数提供了完整的Vulkan缓冲区管理功能。

## 功能概述

### 1. 基础缓冲区操作
- `createBuffer()` - 创建Vulkan缓冲区
- `allocateBufferMemory()` - 为缓冲区分配内存
- `bindBufferMemory()` - 绑定缓冲区内存

### 2. 缓冲区上传功能
- `uploadDataToBuffer()` - 通用数据上传函数
- `uploadDataWithStaging()` - 使用暂存缓冲区的上传
- `uploadDataDirect()` - 直接映射上传（适用于主机可见内存）

### 3. 暂存缓冲区管理器
`StagingBufferManager` 类提供了高效的暂存缓冲区管理：
- 自动创建和销毁暂存缓冲区
- 支持批量上传操作
- 内存使用优化

### 4. 缓冲区复制
- `copyBuffer()` - 缓冲区到缓冲区复制
- `copyBufferToImage()` - 缓冲区到图像复制
- `batchCopyBuffers()` - 批量复制操作

### 5. 内存管理工具
- `alignSize()` - 内存对齐计算
- `getBufferAlignment()` - 获取缓冲区对齐要求
- `mapMemory()` / `unmapMemory()` - 内存映射操作
- `flushMappedMemory()` / `invalidateMappedMemory()` - 内存同步

### 6. 缓冲区池管理
`BufferPool` 类提供了缓冲区池功能：
- 预分配缓冲区池
- 动态分配和释放
- 减少内存碎片

### 7. 验证和调试工具
- `validateBuffer()` - 缓冲区有效性验证
- `printBufferInfo()` - 打印缓冲区信息
- `checkBufferSupport()` - 检查设备支持
- `getBufferUsageString()` - 获取使用标志字符串

## 使用示例

### 创建顶点缓冲区

```cpp
// 创建顶点缓冲区
VkBuffer vertex_buffer = GPUBufferUtility::createBuffer(
    device, 
    vertices.size() * sizeof(Vertex), 
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
);

// 分配内存
VkDeviceMemory vertex_memory = GPUBufferUtility::allocateBufferMemory(
    device, 
    physical_device, 
    vertex_buffer, 
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
);

// 绑定内存
GPUBufferUtility::bindBufferMemory(device, vertex_buffer, vertex_memory);

// 上传数据
GPUBufferUtility::uploadDataToBuffer(
    device, 
    physical_device, 
    command_pool, 
    queue, 
    vertex_buffer, 
    vertices.data(), 
    vertices.size() * sizeof(Vertex)
);
```

### 使用暂存缓冲区管理器

```cpp
// 创建管理器
StagingBufferManager staging_manager(device, physical_device, command_pool, queue);

// 上传数据
staging_manager.uploadToBuffer(dst_buffer, data.data(), data.size());

// 批量上传
std::vector<BufferUploadInfo> uploads = {
    {buffer1, memory1, size1, 0, data1, true},
    {buffer2, memory2, size2, 0, data2, false}
};
staging_manager.uploadMultipleBuffers(uploads);

// 清理
staging_manager.cleanup();
```

### 使用缓冲区池

```cpp
// 创建缓冲区池
GPUBufferUtility::BufferPool uniform_pool(
    device, 
    physical_device, 
    256, // 每个缓冲区256字节
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
);

// 分配缓冲区
VkBuffer buffer = uniform_pool.allocateBuffer();

// 使用缓冲区...

// 释放缓冲区
uniform_pool.freeBuffer(buffer);

// 清理
uniform_pool.cleanup();
```

### 批量操作

```cpp
// 准备批量上传数据
std::vector<std::pair<VkBuffer, std::pair<void*, size_t>>> uploads = {
    {buffer1, {data1, size1}},
    {buffer2, {data2, size2}},
    {buffer3, {data3, size3}}
};

// 批量上传
GPUBufferUtility::batchUploadBuffers(device, physical_device, command_pool, queue, uploads);
```

## 性能优化建议

### 1. 使用暂存缓冲区管理器
对于频繁的数据上传操作，使用`StagingBufferManager`可以：
- 减少内存分配开销
- 优化内存使用
- 提供更好的性能

### 2. 批量操作
使用批量操作函数可以减少命令缓冲区提交次数：
- `batchUploadBuffers()` - 批量上传
- `batchCopyBuffers()` - 批量复制

### 3. 缓冲区池
对于相同大小的缓冲区，使用`BufferPool`可以：
- 减少内存碎片
- 提高分配效率
- 简化内存管理

### 4. 内存对齐
始终使用`alignSize()`函数确保内存对齐：
```cpp
size_t aligned_size = GPUBufferUtility::alignSize(size, alignment);
```

## 错误处理

所有函数都包含适当的错误检查：
- 返回`VK_NULL_HANDLE`表示失败
- 使用日志系统记录错误信息
- 提供详细的错误消息

## 注意事项

1. **内存管理**: 确保正确释放所有分配的内存
2. **命令缓冲区**: 某些操作需要有效的命令缓冲区和队列
3. **设备支持**: 使用前检查设备是否支持所需功能
4. **同步**: 注意GPU和CPU之间的同步需求

## 扩展功能

这些工具函数为以下高级功能提供了基础：
- 动态缓冲区管理
- 多线程缓冲区操作
- 内存池优化
- 异步数据传输

通过合理使用这些工具函数，可以大大简化Vulkan缓冲区管理，提高开发效率并优化性能。
