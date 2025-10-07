# PiccoloDev 映射容器序列化支持

本文档描述了PiccoloDev反射系统对`std::map`和`std::unordered_map`容器的序列化和反序列化支持。

## 概述

PiccoloDev的序列化系统现在完全支持以下映射容器类型的序列化：
- `std::map<Key, Value>`
- `std::unordered_map<Key, Value>`

映射容器在JSON中被序列化为对象，其中键被转换为字符串，值保持其原始类型。

## 序列化格式

### 基本映射容器

映射容器在JSON中被序列化为对象格式：

```cpp
std::map<std::string, int> m_string_int_map;
// 数据: {"hello": 42, "world": 100}
// JSON: {"m_string_int_map": {"hello": 42, "world": 100}}
```

```cpp
std::map<int, std::string> m_int_string_map;
// 数据: {1: "one", 2: "two"}
// JSON: {"m_int_string_map": {"1": "one", "2": "two"}}
```

### 嵌套容器

支持复杂的嵌套容器结构：

```cpp
std::map<std::string, std::vector<int>> m_string_vector_map;
// 数据: {"numbers": [1, 2, 3], "primes": [2, 3, 5]}
// JSON: {"m_string_vector_map": {"numbers": [1, 2, 3], "primes": [2, 3, 5]}}
```

```cpp
std::unordered_map<int, std::map<std::string, float>> m_nested_map;
// 数据: {1: {"first": 1.0, "second": 2.0}}
// JSON: {"m_nested_map": {"1": {"first": 1.0, "second": 2.0}}}
```

## 支持的键类型

序列化系统支持以下键类型：

### 基本类型
- `std::string` - 直接作为JSON对象键
- `int`, `unsigned int` - 转换为字符串键
- `long`, `unsigned long` - 转换为字符串键
- `long long`, `unsigned long long` - 转换为字符串键
- `char` - 转换为字符串键
- `bool` - 转换为字符串键（"true"/"false"）

### 浮点类型
- `float`, `double` - 转换为字符串键

### 自定义类型
- 任何支持序列化的自定义类型都可以作为键

## 使用方法

### 1. 定义映射容器字段

```cpp
#include "runtime/core/meta/reflection/reflection.h"

namespace Piccolo
{
    REFLECTION_TYPE(MyClass)
    CLASS(MyClass, Fields)
    {
        REFLECTION_BODY(MyClass);

    public:
        // 基本映射容器
        std::map<std::string, int> m_string_int_map;
        std::unordered_map<int, std::string> m_int_string_map;
        
        // 嵌套容器
        std::map<std::string, std::vector<int>> m_string_vector_map;
        std::unordered_map<int, std::map<std::string, float>> m_nested_map;
    };
}
```

### 2. 序列化映射容器

```cpp
#include "runtime/core/meta/reflection/reflection.h"
#include "_generated/serializer/all_serializer.h"

void serializeMapContainer()
{
    // 注册反射类型
    Reflection::TypeMetaRegister::metaRegister();
    
    // 创建实例并填充数据
    MyClass instance;
    instance.m_string_int_map["hello"] = 42;
    instance.m_string_int_map["world"] = 100;
    instance.m_int_string_map[1] = "one";
    instance.m_int_string_map[2] = "two";
    
    // 序列化到JSON
    auto json_result = Serializer::write(instance);
    std::string json_string = json_result.dump();
    
    std::cout << "Serialized JSON:" << std::endl;
    std::cout << json_string << std::endl;
    
    // 保存到文件
    std::ofstream out_file("data.json");
    out_file << json_string;
    out_file.close();
}
```

### 3. 反序列化映射容器

```cpp
void deserializeMapContainer()
{
    // 从文件读取JSON
    std::ifstream in_file("data.json");
    std::string json_string((std::istreambuf_iterator<char>(in_file)),
                           std::istreambuf_iterator<char>());
    
    // 解析JSON
    std::string error;
    auto parsed_json = Json::parse(json_string, error);
    
    if (!error.empty())
    {
        std::cerr << "JSON parse error: " << error << std::endl;
        return;
    }
    
    // 反序列化
    MyClass deserialized_instance;
    Serializer::read(parsed_json, deserialized_instance);
    
    // 验证数据
    std::cout << "Deserialized data:" << std::endl;
    std::cout << "m_string_int_map[\"hello\"]: " 
              << deserialized_instance.m_string_int_map["hello"] << std::endl;
    std::cout << "m_int_string_map[1]: " 
              << deserialized_instance.m_int_string_map[1] << std::endl;
}
```

## 技术实现

### 序列化生成器扩展

- 在`serializer_generator.cpp`中添加了映射容器类型识别
- 扩展了`genClassFieldRenderData`方法以支持映射容器字段
- 为映射容器添加了键类型和值类型信息

### 模板系统扩展

- 更新了`allSerializer.ipp.mustache`模板以支持映射容器序列化
- 序列化：将映射容器转换为JSON对象
- 反序列化：从JSON对象重建映射容器

### 序列化器扩展

- 添加了对更多基本类型的支持（`long long`, `unsigned long long`, `long`, `unsigned long`）
- 支持所有常用键类型的序列化和反序列化

## 序列化代码生成示例

对于映射容器字段，生成的序列化代码如下：

```cpp
// 序列化
Json::object m_string_int_map_json;
for (auto& pair : instance.m_string_int_map){
    m_string_int_map_json.insert_or_assign(Serializer::write(pair.first).string_value(), Serializer::write(pair.second));
}
ret_context.insert_or_assign("m_string_int_map", m_string_int_map_json);

// 反序列化
assert(json_context["m_string_int_map"].is_object());
Json::object object_m_string_int_map = json_context["m_string_int_map"].object_items();
instance.m_string_int_map.clear();
for (auto& pair : object_m_string_int_map){
    std::string key;
    int value;
    Serializer::read(Json(pair.first), key);
    Serializer::read(pair.second, value);
    instance.m_string_int_map[key] = value;
}
```

## 限制和注意事项

1. **键类型转换**: 非字符串键类型会被转换为字符串存储在JSON中
2. **浮点精度**: 浮点数键可能存在精度损失
3. **自定义键类型**: 自定义类型作为键需要支持序列化
4. **性能**: 大型映射容器的序列化可能较慢
5. **内存使用**: 反序列化时会创建临时对象

## 测试示例

参考以下文件了解完整的使用示例：
- `engine/source/runtime/core/meta/map_serialization_test.h` - 测试类定义
- `engine/source/runtime/core/meta/map_serialization_test.cpp` - 完整测试程序

## 构建说明

确保在构建时包含以下更新的文件：
- 更新的序列化生成器
- 更新的序列化模板
- 扩展的序列化器实现
- 新的基本类型支持

映射容器序列化支持与现有的数组和基本类型序列化完全兼容，可以混合使用。
