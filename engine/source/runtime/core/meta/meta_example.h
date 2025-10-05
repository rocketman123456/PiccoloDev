#pragma once

#include "runtime/core/meta/reflection/reflection.h"

#include <map>
#include <unordered_map>
#include <vector>

namespace Piccolo
{
    // 简单枚举示例
    enum class TestEnum : int
    {
        Value1 = 1,
        Value2 = 2,
        Value3 = 3
    };

    // 另一个枚举示例
    enum class StatusEnum
    {
        Pending,
        Running,
        Completed,
        Failed
    };

    REFLECTION_TYPE(BaseTest)
    CLASS(BaseTest, Fields)
    {
        REFLECTION_BODY(BaseTest);

    public:
        int               m_int;
        std::vector<int*> m_int_vector;
    };

    REFLECTION_TYPE(Test1)
    CLASS(Test1 : public BaseTest, WhiteListFields)
    {
        REFLECTION_BODY(Test1);

    public:
        META(Enable)
        char m_char;
    };

    REFLECTION_TYPE(Test2)
    CLASS(Test2 : public BaseTest, , Fields)
    {
        REFLECTION_BODY(Test2);

    public:
        std::vector<int>                                 m_int_vector;
        std::vector<Reflection::ReflectionPtr<BaseTest>> m_test_base_array;
    };

    REFLECTION_TYPE(Test3)
    CLASS(Test3, Fields)
    {
        REFLECTION_BODY(BaseTest);

    public:
        int                          m_int;
        std::map<int, int>           m_int_map;
        std::unordered_map<int, int> m_int_unordered_map;
    };

    // 包含枚举字段的测试类
    REFLECTION_TYPE(TestWithEnum)
    CLASS(TestWithEnum, Fields)
    {
        REFLECTION_BODY(TestWithEnum);

    public:
        TestEnum   m_test_enum;   // 带值的枚举
        StatusEnum m_status_enum; // 默认值枚举
        int        m_regular_int; // 普通字段
    };

    void metaExample();
} // namespace Piccolo
