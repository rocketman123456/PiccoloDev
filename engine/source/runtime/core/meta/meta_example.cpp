#include "runtime/core/meta/meta_example.h"
#include "_generated/serializer/all_serializer.h"

#include "runtime/core/base/macro.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace Piccolo
{
    void metaExample()
    {
        Test1 test1_in;
        test1_in.m_int  = 12;
        test1_in.m_char = 'g';
        // Note: We'll skip the int* vector for now to avoid pointer serialization issues
        // int i           = 1;
        // test1_in.m_int_vector.emplace_back(&i);

        Test1 test1_out;
        // test on array
        Test2 test2_in;
        test2_in.m_test_base_array.emplace_back("Test1", &test1_in);
        Test1 Test2_temp;
        test2_in.m_test_base_array.emplace_back("Test1", &Test2_temp);

        // serializer & deserializer

        // write Test1_in (object) to Test1_json_in (json)
        auto test1_json_in = Serializer::write(test1_in);

        std::string test1_context = test1_json_in.dump();

        // read Test1_context (json) to Test1_out (object)
        std::string err;

        auto&& Test1_json = Json::parse(test1_context, err);
        Serializer::read(Test1_json, test1_out);
        LOG_INFO(test1_context);

        auto        Test2_json_in = Serializer::write(test2_in);
        std::string test2_context = Test2_json_in.dump();

        std::fstream out_put("out.txt", std::ios::out);
        out_put << test2_context;
        out_put.flush();
        out_put.close();

        Test2  test2_out;
        auto&& test2_json = Json::parse(test2_context, err);
        Serializer::read(test2_json, test2_out);
        LOG_INFO(test2_context.c_str());

        // Test3 example with std::map and std::unordered_map
        Test3 test3_in;
        test3_in.m_int = 42;
        
        // Add some data to the map
        test3_in.m_int_map[1] = 100;
        test3_in.m_int_map[2] = 200;
        test3_in.m_int_map[3] = 300;
        
        // Add some data to the unordered_map
        test3_in.m_int_unordered_map[10] = 1000;
        test3_in.m_int_unordered_map[20] = 2000;
        test3_in.m_int_unordered_map[30] = 3000;

        // Serialize Test3
        auto test3_json_in = Serializer::write(test3_in);
        std::string test3_context = test3_json_in.dump();
        
        LOG_INFO("Test3 serialized:");
        LOG_INFO(test3_context.c_str());

        // Deserialize Test3
        Test3 test3_out;
        auto&& test3_json = Json::parse(test3_context, err);
        Serializer::read(test3_json, test3_out);
        
        LOG_INFO("Test3 deserialized - m_int: {}", test3_out.m_int);
        LOG_INFO("Test3 map size: {}", test3_out.m_int_map.size());
        LOG_INFO("Test3 unordered_map size: {}", test3_out.m_int_unordered_map.size());
        
        // Verify map contents
        for (const auto& pair : test3_out.m_int_map)
        {
            LOG_INFO("Map[{}] = {}", pair.first, pair.second);
        }
        
        // Verify unordered_map contents
        for (const auto& pair : test3_out.m_int_unordered_map)
        {
            LOG_INFO("UnorderedMap[{}] = {}", pair.first, pair.second);
        }

        // reflection
        auto                       meta = TypeMetaDef(Test2, &test2_out);
        Reflection::FieldAccessor* fields;
        int                        fields_count = meta.m_meta.getFieldsList(fields);
        for (int i = 0; i < fields_count; ++i)
        {
            auto filed_accesser = fields[i];
            std::cout << filed_accesser.getFieldTypeName() << " " << filed_accesser.getFieldName() << " " << (char*)filed_accesser.get(meta.m_instance) << "\n";

            if (filed_accesser.isArrayType())
            {
                Reflection::ArrayAccessor array_accesser;
                if (Reflection::TypeMeta::newArrayAccessorFromName(filed_accesser.getFieldTypeName(), array_accesser))
                {
                    void* field_instance = filed_accesser.get(meta.m_instance);
                    int   count          = array_accesser.getSize(field_instance);
                    auto  typeMetaItem   = Reflection::TypeMeta::newMetaFromName(array_accesser.getElementTypeName());
                    for (int index = 0; index < count; ++index)
                    {
                        std::cout << ":L:" << index << ":R:" << (int*)array_accesser.get(index, field_instance) << std::endl;
                    }
                }
            }
        }
    }
} // namespace Piccolo
