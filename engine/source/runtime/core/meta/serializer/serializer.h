#pragma once
#include "runtime/core/meta/json.h"
#include "runtime/core/meta/reflection/reflection.h"

#include <cassert>
#include <map>
#include <unordered_map>
#include <vector>

// Forward declarations for enums
namespace Piccolo
{
    enum class TestEnum : int;
    enum class StatusEnum;
} // namespace Piccolo

namespace Piccolo
{
    template<typename...>
    inline constexpr bool always_false = false;

    class Serializer
    {
    public:
        template<typename T>
        static Json writePointer(T* instance)
        {
            return Json::object {
                {"$typeName",                   Json {"*"}},
                { "$context", Serializer::write(*instance)}
            };
        }

        template<typename T>
        static T*& readPointer(const Json& json_context, T*& instance)
        {
            assert(instance == nullptr);
            std::string type_name = json_context["$typeName"].string_value();
            assert(!type_name.empty());
            if ('*' == type_name[0])
            {
                instance = new T;
                read(json_context["$context"], *instance);
            }
            else
            {
                instance = static_cast<T*>(Reflection::TypeMeta::newFromNameAndJson(type_name, json_context["$context"]).m_instance);
            }
            return instance;
        }

        template<typename T>
        static Json write(const Reflection::ReflectionPtr<T>& instance)
        {
            T*          instance_ptr = static_cast<T*>(instance.operator->());
            std::string type_name    = instance.getTypeName();
            return Json::object {
                {"$typeName", Json(type_name)},
                {"$context", Reflection::TypeMeta::writeByName(type_name, instance_ptr)}
            };
        }

        template<typename T>
        static T*& read(const Json& json_context, Reflection::ReflectionPtr<T>& instance)
        {
            std::string type_name = json_context["$typeName"].string_value();
            instance.setTypeName(type_name);
            // Clear the pointer before reading to satisfy readPointer's assertion
            instance.getPtrReference() = nullptr;
            return readPointer(json_context, instance.getPtrReference());
        }

        template<typename T>
        static Json write(const T& instance)
        {

            if constexpr (std::is_pointer<T>::value)
            {
                return writePointer(static_cast<T>(instance));
            }
            else
            {
                static_assert(always_false<T>, "Serializer::write<T> has not been implemented yet!");
                return Json();
            }
        }

        template<typename T>
        static T& read(const Json& json_context, T& instance)
        {
            if constexpr (std::is_pointer<T>::value)
            {
                return readPointer(json_context, instance);
            }
            else
            {
                static_assert(always_false<T>, "Serializer::read<T> has not been implemented yet!");
                return instance;
            }
        }

        // std::vector specialization
        template<typename T>
        static Json write(const std::vector<T>& instance)
        {
            Json::array array_json;
            for (const auto& item : instance)
            {
                array_json.emplace_back(Serializer::write(item));
            }
            return Json(array_json);
        }

        template<typename T>
        static std::vector<T>& read(const Json& json_context, std::vector<T>& instance)
        {
            assert(json_context.is_array());
            Json::array array_items = json_context.array_items();
            instance.resize(array_items.size());
            for (size_t index = 0; index < array_items.size(); ++index)
            {
                Serializer::read(array_items[index], instance[index]);
            }
            return instance;
        }

        // std::map specialization
        template<typename K, typename V>
        static Json write(const std::map<K, V>& instance)
        {
            Json::object map_json;
            for (const auto& pair : instance)
            {
                std::string key_str = std::to_string(pair.first);
                map_json[key_str]   = Serializer::write(pair.second);
            }
            return Json(map_json);
        }

        template<typename K, typename V>
        static std::map<K, V>& read(const Json& json_context, std::map<K, V>& instance)
        {
            assert(json_context.is_object());
            instance.clear();
            Json::object map_items = json_context.object_items();
            for (const auto& pair : map_items)
            {
                K key = static_cast<K>(std::stoi(pair.first));
                V value;
                Serializer::read(pair.second, value);
                instance[key] = value;
            }
            return instance;
        }

        // std::unordered_map specialization
        template<typename K, typename V>
        static Json write(const std::unordered_map<K, V>& instance)
        {
            Json::object map_json;
            for (const auto& pair : instance)
            {
                std::string key_str = std::to_string(pair.first);
                map_json[key_str]   = Serializer::write(pair.second);
            }
            return Json(map_json);
        }

        template<typename K, typename V>
        static std::unordered_map<K, V>& read(const Json& json_context, std::unordered_map<K, V>& instance)
        {
            assert(json_context.is_object());
            instance.clear();
            Json::object map_items = json_context.object_items();
            for (const auto& pair : map_items)
            {
                K key = static_cast<K>(std::stoi(pair.first));
                V value;
                Serializer::read(pair.second, value);
                instance[key] = value;
            }
            return instance;
        }
    };

    // implementation of base types
    template<>
    Json Serializer::write(const char& instance);
    template<>
    char& Serializer::read(const Json& json_context, char& instance);

    template<>
    Json Serializer::write(const int& instance);
    template<>
    int& Serializer::read(const Json& json_context, int& instance);

    template<>
    Json Serializer::write(const unsigned int& instance);
    template<>
    unsigned int& Serializer::read(const Json& json_context, unsigned int& instance);

    template<>
    Json Serializer::write(const float& instance);
    template<>
    float& Serializer::read(const Json& json_context, float& instance);

    template<>
    Json Serializer::write(const double& instance);
    template<>
    double& Serializer::read(const Json& json_context, double& instance);

    template<>
    Json Serializer::write(const bool& instance);
    template<>
    bool& Serializer::read(const Json& json_context, bool& instance);

    template<>
    Json Serializer::write(const std::string& instance);
    template<>
    std::string& Serializer::read(const Json& json_context, std::string& instance);

    template<>
    Json Serializer::write(const long long& instance);
    template<>
    long long& Serializer::read(const Json& json_context, long long& instance);

    template<>
    Json Serializer::write(const unsigned long long& instance);
    template<>
    unsigned long long& Serializer::read(const Json& json_context, unsigned long long& instance);

    template<>
    Json Serializer::write(const long& instance);
    template<>
    long& Serializer::read(const Json& json_context, long& instance);

    template<>
    Json Serializer::write(const unsigned long& instance);
    template<>
    unsigned long& Serializer::read(const Json& json_context, unsigned long& instance);

    // Enum specializations
    template<>
    Json Serializer::write(const TestEnum& instance);
    template<>
    TestEnum& Serializer::read(const Json& json_context, TestEnum& instance);

    template<>
    Json Serializer::write(const StatusEnum& instance);
    template<>
    StatusEnum& Serializer::read(const Json& json_context, StatusEnum& instance);

    // template<>
    // Json Serializer::write(const Reflection::object& instance);
    // template<>
    // Reflection::object& Serializer::read(const Json& json_context, Reflection::object& instance);

    ////////////////////////////////////
    ////sample of generation coder
    ////////////////////////////////////
    // class test_class
    //{
    // public:
    //     int a;
    //     unsigned int b;
    //     std::vector<int> c;
    // };
    // class ss;
    // class jkj;
    // template<>
    // Json Serializer::write(const ss& instance);
    // template<>
    // Json Serializer::write(const jkj& instance);

    /*REFLECTION_TYPE(jkj)
    CLASS(jkj,Fields)
    {
        REFLECTION_BODY(jkj);
        int jl;
    };

    REFLECTION_TYPE(ss)
    CLASS(ss:public jkj,WhiteListFields)
    {
        REFLECTION_BODY(ss);
        int jl;
    };*/

    ////////////////////////////////////
    ////template of generation coder
    ////////////////////////////////////
    // template<>
    // Json Serializer::write(const test_class& instance);
    // template<>
    // test_class& Serializer::read(const Json& json_context, test_class& instance);

    //
    ////////////////////////////////////
} // namespace Piccolo
