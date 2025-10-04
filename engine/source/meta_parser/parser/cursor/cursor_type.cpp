#include "common/precompiled.h"

#include "cursor.h"
#include "cursor_type.h"

CursorType::CursorType(const CXType& handle)
    : m_handle(handle)
{}

std::string CursorType::GetDisplayName(void) const
{
    std::string display_name;

    Utils::toString(clang_getTypeSpelling(m_handle), display_name);

    // For template types, try to get a more complete type name
    if (display_name.empty() || (display_name.find('<') == std::string::npos && m_handle.kind == CXType_Unexposed))
    {
        // Try to get the canonical type
        CXType canonical_type = clang_getCanonicalType(m_handle);
        std::string canonical_name;
        Utils::toString(clang_getTypeSpelling(canonical_type), canonical_name);
    
        if (!canonical_name.empty() && canonical_name != display_name)
        {
            display_name = canonical_name;
        }
    }

    return display_name;
}

int CursorType::GetArgumentCount(void) const { return clang_getNumArgTypes(m_handle); }

CursorType CursorType::GetArgument(unsigned index) const { return clang_getArgType(m_handle, index); }

CursorType CursorType::GetCanonicalType(void) const { return clang_getCanonicalType(m_handle); }

Cursor CursorType::GetDeclaration(void) const { return clang_getTypeDeclaration(m_handle); }

CXTypeKind CursorType::GetKind(void) const { return m_handle.kind; }

bool CursorType::IsConst(void) const { return clang_isConstQualifiedType(m_handle) ? true : false; }