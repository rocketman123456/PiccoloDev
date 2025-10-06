#include "editor/include/ui_components/component_inspector.h"
#include "editor/include/ui_components/basic_property_editors.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Piccolo
{
    namespace Editor
    {
        ComponentInspector::ComponentInspector()
        {
            // 注册基础属性编辑器
            m_property_editors.push_back(std::make_unique<BoolPropertyEditor>());
            m_property_editors.push_back(std::make_unique<IntPropertyEditor>());
            m_property_editors.push_back(std::make_unique<FloatPropertyEditor>());
            m_property_editors.push_back(std::make_unique<StringPropertyEditor>());
            m_property_editors.push_back(std::make_unique<Vector3PropertyEditor>());
            m_property_editors.push_back(std::make_unique<QuaternionPropertyEditor>());
        }

        void ComponentInspector::render()
        {
            if (!m_visible || !m_selected_object)
                return;

            ImGui::Begin("组件检查器", &m_visible);

            // 显示对象名称
            ImGui::Text("对象: %s", m_selected_object->getName().c_str());
            ImGui::Separator();

            // 渲染所有组件
            auto&& components = m_selected_object->getComponents();
            for (auto& component_ptr : components)
            {
                std::string component_name = "<" + component_ptr.getTypeName() + ">";

                if (ComponentTreeNodeRenderer::beginComponentNode(component_name))
                {
                    ComponentTreeNodeRenderer::renderComponentIcon(component_ptr.getTypeName());

                    // 创建反射实例
                    auto object_instance =
                        Reflection::ReflectionInstance(Reflection::TypeMeta::newMetaFromName(component_ptr.getTypeName().c_str()), component_ptr.operator->());

                    renderComponentProperties(object_instance);

                    ComponentTreeNodeRenderer::endComponentNode();
                }
            }

            ImGui::End();
        }

        void ComponentInspector::setSelectedObject(std::shared_ptr<GObject> object) { m_selected_object = object; }

        void ComponentInspector::clearSelection() { m_selected_object = nullptr; }

        void ComponentInspector::renderComponentProperties(Reflection::ReflectionInstance& instance)
        {
            // 渲染基类属性
            Reflection::ReflectionInstance* base_instances;
            int                             base_count = instance.m_meta.getBaseClassReflectionInstanceList(base_instances, instance.m_instance);
            for (int i = 0; i < base_count; i++)
            {
                renderComponentProperties(base_instances[i]);
            }

            if (base_count > 0)
                delete[] base_instances;

            // 渲染当前类属性
            Reflection::FieldAccessor* fields;
            int                        field_count = instance.m_meta.getFieldsList(fields);

            if (field_count > 0)
            {
                if (PropertyTableRenderer::beginPropertyTable("Properties"))
                {
                    for (int i = 0; i < field_count; i++)
                    {
                        renderPropertyField(fields[i], instance.m_instance);
                    }
                    PropertyTableRenderer::endPropertyTable();
                }
            }
        }

        void ComponentInspector::renderPropertyField(Reflection::FieldAccessor& field, void* instance_ptr)
        {
            std::string field_name = field.getFieldName();
            std::string field_type = field.getFieldTypeName();
            void*       field_ptr  = field.get(instance_ptr);

            if (field.isArrayType())
            {
                renderArrayProperty(field, instance_ptr);
                return;
            }

            // 查找合适的属性编辑器
            IPropertyEditor* editor = nullptr;
            for (auto& editor_ptr : m_property_editors)
            {
                if (editor_ptr->canEdit(field_type))
                {
                    editor = editor_ptr.get();
                    break;
                }
            }

            if (editor)
            {
                PropertyTableRenderer::renderPropertyRow(field_name, [editor, field_name, field_ptr]() { editor->render(field_name, field_ptr); });
            }
            else
            {
                // 默认显示为只读文本
                PropertyTableRenderer::renderPropertyRow(field_name, [field_type, field_ptr]() { ImGui::Text("类型: %s (未支持编辑)", field_type.c_str()); });
            }
        }

        void ComponentInspector::renderArrayProperty(Reflection::FieldAccessor& field, void* instance_ptr)
        {
            std::string field_name = field.getFieldName();
            std::string field_type = field.getFieldTypeName();

            PropertyTableRenderer::renderPropertyRow(field_name, [field_name, field_type]() { ImGui::Text("数组类型: %s (未支持编辑)", field_type.c_str()); });
        }

        // 属性表格渲染器实现
        bool PropertyTableRenderer::beginPropertyTable(const std::string& table_name)
        {
            ImGuiTableFlags flags =
                ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;

            return ImGui::BeginTable(table_name.c_str(), 2, flags);
        }

        void PropertyTableRenderer::endPropertyTable() { ImGui::EndTable(); }

        void PropertyTableRenderer::renderPropertyRow(const std::string& label, std::function<void()> value_renderer)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label.c_str());

            ImGui::TableSetColumnIndex(1);
            value_renderer();
        }

        // 组件树节点渲染器实现
        bool ComponentTreeNodeRenderer::beginComponentNode(const std::string& component_name, bool* is_open)
        {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_FramePadding;

            return ImGui::TreeNodeEx(component_name.c_str(), flags);
        }

        void ComponentTreeNodeRenderer::endComponentNode() { ImGui::TreePop(); }

        void ComponentTreeNodeRenderer::renderComponentIcon(const std::string& component_type)
        {
            // 根据组件类型显示不同的图标
            const char* icon = "?";
            if (component_type == "TransformComponent")
                icon = "T";
            else if (component_type == "MeshComponent")
                icon = "M";
            else if (component_type == "LightComponent")
                icon = "L";
            else if (component_type == "CameraComponent")
                icon = "C";

            ImGui::SameLine();
            ImGui::Text("%s", icon);
        }
    } // namespace Editor
} // namespace Piccolo
