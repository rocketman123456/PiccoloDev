#include "common/precompiled.h"
#include "enum.h"
#include "meta/meta_utils.h"

Enum::Enum(const Cursor& cursor, const Namespace& current_namespace)
    : TypeInfo(cursor, current_namespace)
    , m_name(cursor.getDisplayName())
    , m_qualified_name(Utils::getTypeNameWithoutNamespace(cursor.getType()))
    , m_display_name(Utils::getNameWithoutFirstM(m_qualified_name))
{
    Utils::replaceAll(m_name, " ", "");
    Utils::replaceAll(m_name, "Piccolo::", "");

    // 解析枚举值
    for (auto& child : cursor.getChildren())
    {
        if (child.getKind() == CXCursor_EnumConstantDecl)
        {
            std::string enum_name = child.getSpelling();
            std::string enum_value = std::to_string(clang_getEnumConstantDeclValue(child.getHandle()));
            m_values.emplace_back(EnumValue(enum_name, enum_value));
        }
    }
}

bool Enum::shouldCompile(void) const
{
    return m_meta_data.getFlag(NativeProperty::All) || m_meta_data.getFlag(NativeProperty::Enum);
}
