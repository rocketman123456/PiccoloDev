#pragma once

#include "runtime/core/meta/reflection/reflection.h"
#include <functional>
#include <imgui.h>
#include <memory>
#include <string>

namespace Piccolo
{
    namespace Editor
    {
        // UI组件基类
        class UIComponentBase
        {
        public:
            virtual ~UIComponentBase() = default;
            virtual void render()      = 0;
            virtual void setVisible(bool visible) { m_visible = visible; }
            virtual bool isVisible() const { return m_visible; }

        protected:
            bool m_visible = true;
        };

        // 属性编辑器接口
        class IPropertyEditor
        {
        public:
            virtual ~IPropertyEditor()                                     = default;
            virtual void render(const std::string& label, void* value_ptr) = 0;
            virtual bool canEdit(const std::string& type_name) const       = 0;
        };

        // 属性编辑器工厂
        class PropertyEditorFactory
        {
        public:
            using EditorCreator = std::function<std::unique_ptr<IPropertyEditor>()>;

            static PropertyEditorFactory& getInstance();

            void                             registerEditor(const std::string& type_name, EditorCreator creator);
            std::unique_ptr<IPropertyEditor> createEditor(const std::string& type_name);

        private:
            std::unordered_map<std::string, EditorCreator> m_editors;
        };

        // 模板属性编辑器基类
        template<typename T>
        class PropertyEditor : public IPropertyEditor
        {
        public:
            virtual void render(const std::string& label, void* value_ptr) override { renderImpl(label, static_cast<T*>(value_ptr)); }

            virtual bool canEdit(const std::string& type_name) const override { return type_name == getTypeName(); }

        protected:
            virtual void        renderImpl(const std::string& label, T* value) = 0;
            virtual std::string getTypeName() const                            = 0;
        };

// 自动注册宏
#define REGISTER_PROPERTY_EDITOR(Type, EditorClass) \
    static bool s_##EditorClass##_registered = []() { \
        PropertyEditorFactory::getInstance().registerEditor(#Type, []() { return std::make_unique<EditorClass>(); }); \
        return true; \
    }();
    } // namespace Editor
} // namespace Piccolo
