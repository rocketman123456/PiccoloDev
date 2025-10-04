#pragma once
#include "type_info.h"
#include <string>
#include <vector>

struct EnumValue
{
    std::string name;
    std::string value;

    EnumValue(const std::string& n, const std::string& v)
        : name(n)
        , value(v)
    {}
};

class Enum : public TypeInfo
{
public:
    Enum(const Cursor& cursor, const Namespace& current_namespace);
    virtual ~Enum(void) {}

    const std::string& getName(void) const { return m_name; }
    const std::string& getQualifiedName(void) const { return m_qualified_name; }
    const std::string& getDisplayName(void) const { return m_display_name; }

    const std::vector<EnumValue>& getValues(void) const { return m_values; }

    bool shouldCompile(void) const;
    bool isAccessible(void) const { return m_enabled; }

public:
    std::string            m_name;
    std::string            m_qualified_name;
    std::string            m_display_name;
    std::vector<EnumValue> m_values;
};
