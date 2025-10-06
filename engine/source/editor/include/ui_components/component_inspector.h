#pragma once

#include "runtime/core/meta/reflection/reflection.h"
#include "runtime/function/framework/object/object.h"

#include "editor/include/ui_components/ui_component_base.h"

#include <imgui.h>
#include <memory>
#include <vector>

namespace Piccolo
{
    namespace Editor
    {
        // 组件检查器 - 用于显示和编辑组件属性
        class ComponentInspector : public UIComponentBase
        {
        public:
            ComponentInspector();
            ~ComponentInspector() override = default;

            void render() override;
            void setSelectedObject(std::shared_ptr<GObject> object);
            void clearSelection();

        private:
            void renderComponentHeader(const std::string& component_name);
            void renderComponentProperties(Reflection::ReflectionInstance& instance);
            void renderPropertyField(Reflection::FieldAccessor& field, void* instance_ptr);
            void renderArrayProperty(Reflection::FieldAccessor& field, void* instance_ptr);

            std::shared_ptr<GObject>                      m_selected_object;
            std::vector<std::unique_ptr<IPropertyEditor>> m_property_editors;
        };

        // 属性表格渲染器
        class PropertyTableRenderer
        {
        public:
            static bool beginPropertyTable(const std::string& table_name);
            static void endPropertyTable();
            static void renderPropertyRow(const std::string& label, std::function<void()> value_renderer);
        };

        // 组件树节点渲染器
        class ComponentTreeNodeRenderer
        {
        public:
            static bool beginComponentNode(const std::string& component_name, bool* is_open = nullptr);
            static void endComponentNode();
            static void renderComponentIcon(const std::string& component_type);
        };
    } // namespace Editor
} // namespace Piccolo
