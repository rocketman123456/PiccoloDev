#include "editor/include/ui_components/ui_component_base.h"

namespace Piccolo
{
    namespace Editor
    {
        // 属性编辑器工厂实现
        PropertyEditorFactory& PropertyEditorFactory::getInstance()
        {
            static PropertyEditorFactory instance;
            return instance;
        }

        void PropertyEditorFactory::registerEditor(const std::string& type_name, EditorCreator creator)
        {
            m_editors[type_name] = creator;
        }

        std::unique_ptr<IPropertyEditor> PropertyEditorFactory::createEditor(const std::string& type_name)
        {
            auto it = m_editors.find(type_name);
            if (it != m_editors.end())
            {
                return it->second();
            }
            return nullptr;
        }
    }
}
