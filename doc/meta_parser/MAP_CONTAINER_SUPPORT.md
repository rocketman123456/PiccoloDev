# PiccoloDev 映射容器反射支持

本文档描述了PiccoloDev反射系统对`std::map`和`std::unordered_map`容器的支持。

## 概述

PiccoloDev的反射系统现在支持以下映射容器类型：
- `std::map<Key, Value>`
- `std::unordered_map<Key, Value>`

这些容器类型可以像其他字段一样进行反射，支持运行时类型查询、序列化和动态访问。

## 使用方法

### 1. 定义映射容器字段

在您的类中使用`CLASS`宏和`REFLECTION_BODY`宏来标记需要反射的类：

```cpp
#include "runtime/core/meta/reflection/reflection.h"

namespace Piccolo
{
    REFLECTION_TYPE(MyClass)
    CLASS(MyClass, Fields)
    {
        REFLECTION_BODY(MyClass);

    public:
        // 支持 std::map
        std::map<std::string, int> m_string_int_map;
        std::map<int, std::string> m_int_string_map;
        
        // 支持 std::unordered_map
        std::unordered_map<std::string, float> m_string_float_unordered_map;
        std::unordered_map<int, double> m_int_double_unordered_map;
        
        // 支持嵌套容器
        std::map<std::string, std::vector<int>> m_string_vector_map;
        std::unordered_map<int, std::map<std::string, float>> m_int_map_unordered_map;
    };
}
```

### 2. 运行时访问映射容器

使用`MapAccessor`类来访问映射容器：

```cpp
#include "runtime/core/meta/reflection/reflection.h"

void useMapReflection()
{
    // 注册反射类型
    Reflection::TypeMetaRegister::metaRegister();
    
    // 创建实例
    MyClass instance;
    
    // 获取类型元数据
    Reflection::TypeMeta meta = Reflection::TypeMeta::newMetaFromName("MyClass");
    
    if (meta.isValid())
    {
        // 获取字段访问器
        Reflection::FieldAccessor* fields;
        int field_count = meta.getFieldsList(fields);
        
        for (int i = 0; i < field_count; ++i)
        {
            const char* field_name = fields[i].getFieldName();
            const char* field_type = fields[i].getFieldTypeName();
            
            // 检查是否为映射容器
            if (fields[i].isArray()) // 映射容器被标记为数组类型
            {
                // 获取映射访问器
                Reflection::MapAccessor map_accessor;
                if (Reflection::TypeMeta::newMapAccessorFromName(field_type, map_accessor))
                {
                    // 获取字段实例
                    void* field_instance = fields[i].get(&instance);
                    
                    if (field_instance)
                    {
                        // 获取映射大小
                        int map_size = map_accessor.getSize(field_instance);
                        
                        // 检查键是否存在
                        std::string test_key = "test";
                        if (map_accessor.hasKey(&test_key, field_instance))
                        {
                            // 获取值
                            void* value_ptr = map_accessor.getValue(&test_key, field_instance);
                            if (value_ptr)
                            {
                                // 根据值类型进行类型转换
                                // 例如：int value = *static_cast<int*>(value_ptr);
                            }
                        }
                        
                        // 设置值
                        std::string new_key = "new_key";
                        int new_value = 42;
                        map_accessor.setValue(&new_key, &new_value, field_instance);
                        
                        // 删除键
                        map_accessor.removeKey(&test_key, field_instance);
                    }
                }
            }
        }
        
        delete[] fields;
    }
}
```

### 3. MapAccessor API

`MapAccessor`类提供以下方法：

- `const char* getMapTypeName()` - 获取映射容器的完整类型名称
- `const char* getKeyTypeName()` - 获取键的类型名称
- `const char* getValueTypeName()` - 获取值的类型名称
- `int getSize(void* instance)` - 获取映射容器的大小
- `bool hasKey(void* key, void* instance)` - 检查键是否存在
- `void* getValue(void* key, void* instance)` - 获取指定键的值
- `void setValue(void* key, void* value, void* instance)` - 设置键值对
- `void removeKey(void* key, void* instance)` - 删除指定的键

## 技术实现

### 解析器扩展

- 在`meta_utils.h/cpp`中添加了映射容器类型识别函数：
  - `isMapContainer()` - 识别`std::map`类型
  - `isUnorderedMapContainer()` - 识别`std::unordered_map`类型
  - `getMapKeyType()` - 提取键类型
  - `getMapValueType()` - 提取值类型

### 代码生成器扩展

- 在`reflection_generator.cpp`中添加了映射容器的处理逻辑
- 为每个映射容器类型生成对应的操作符类
- 生成映射容器的注册代码

### 模板系统扩展

- 在`commonReflectionFile.mustache`中添加了映射容器的模板支持
- 生成`MapReflectionOperator`命名空间和操作符类
- 支持映射容器的注册宏

### 运行时系统扩展

- 添加了`MapAccessor`类用于运行时访问映射容器
- 添加了`MapFunctionTuple`类型定义
- 扩展了`TypeMetaRegisterinterface`以支持映射容器注册
- 添加了`newMapAccessorFromName`静态方法

## 限制和注意事项

1. **类型安全**: 运行时访问需要手动进行类型转换，请确保类型匹配
2. **键类型**: 键类型必须支持比较操作（对于`std::map`）或哈希操作（对于`std::unordered_map`）
3. **嵌套容器**: 支持嵌套容器，但访问嵌套容器需要额外的类型处理
4. **性能**: 反射访问比直接访问稍慢，建议在性能关键路径上谨慎使用

## 示例项目

参考以下文件了解完整的使用示例：
- `engine/source/runtime/core/meta/map_test_example.h` - 测试类定义
- `engine/source/runtime/core/meta/map_usage_example.cpp` - 使用示例

## 构建说明

确保在构建时包含以下文件：
- 更新的反射系统头文件和实现文件
- 新的工具函数实现
- 更新的模板文件

映射容器支持与现有的数组支持完全兼容，可以同时使用。
