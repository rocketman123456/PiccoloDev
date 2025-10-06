#include "editor/include/ui_components/basic_property_editors.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace Piccolo
{
    namespace Editor
    {
        // 布尔值编辑器实现
        void BoolPropertyEditor::renderImpl(const std::string& label, bool* value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label.c_str());
            
            ImGui::TableSetColumnIndex(1);
            std::string checkbox_id = "##" + label;
            ImGui::Checkbox(checkbox_id.c_str(), value);
        }

        // 整数编辑器实现
        void IntPropertyEditor::renderImpl(const std::string& label, int* value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label.c_str());
            
            ImGui::TableSetColumnIndex(1);
            std::string input_id = "##" + label;
            ImGui::InputInt(input_id.c_str(), value);
        }

        // 浮点数编辑器实现
        void FloatPropertyEditor::renderImpl(const std::string& label, float* value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label.c_str());
            
            ImGui::TableSetColumnIndex(1);
            std::string input_id = "##" + label;
            ImGui::InputFloat(input_id.c_str(), value, 0.1f, 1.0f, "%.3f");
        }

        // 字符串编辑器实现
        void StringPropertyEditor::renderImpl(const std::string& label, std::string* value)
        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", label.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", value->c_str());
        }

        // Vector3编辑器构造函数
        Vector3PropertyEditor::Vector3PropertyEditor(float reset_value, float column_width)
            : m_reset_value(reset_value), m_column_width(column_width)
        {
        }

        // Vector3编辑器实现
        void Vector3PropertyEditor::renderImpl(const std::string& label, Vector3* value)
        {
            DragControl::drawVec3Control(label, *value, m_reset_value, m_column_width);
        }

        // Quaternion编辑器构造函数
        QuaternionPropertyEditor::QuaternionPropertyEditor(float reset_value, float column_width)
            : m_reset_value(reset_value), m_column_width(column_width)
        {
        }

        // Quaternion编辑器实现
        void QuaternionPropertyEditor::renderImpl(const std::string& label, Quaternion* value)
        {
            DragControl::drawQuatControl(label, *value, m_reset_value, m_column_width);
        }

        // 通用拖拽控件实现
        bool DragControl::drawVec3Control(const std::string& label, Vector3& values, 
                                        float reset_value, float column_width)
        {
            bool changed = false;
            
            ImGui::PushID(label.c_str());
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, column_width);
            ImGui::Text("%s", label.c_str());
            ImGui::NextColumn();
            
            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
            
            float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImVec2 button_size = {line_height + 3.0f, line_height};
            
            // X轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            if (ImGui::Button("X", button_size))
            {
                values.x = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f"))
                changed = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Y轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            if (ImGui::Button("Y", button_size))
            {
                values.y = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f"))
                changed = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Z轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            if (ImGui::Button("Z", button_size))
            {
                values.z = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f"))
                changed = true;
            ImGui::PopItemWidth();
            
            ImGui::PopStyleVar();
            ImGui::Columns(1);
            ImGui::PopID();
            
            return changed;
        }

        bool DragControl::drawQuatControl(const std::string& label, Quaternion& values, 
                                        float reset_value, float column_width)
        {
            bool changed = false;
            
            ImGui::PushID(label.c_str());
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, column_width);
            ImGui::Text("%s", label.c_str());
            ImGui::NextColumn();
            
            ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
            
            float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImVec2 button_size = {line_height + 3.0f, line_height};
            
            // X轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            if (ImGui::Button("X", button_size))
            {
                values.x = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &values.x, 0.01f, -1.0f, 1.0f, "%.3f"))
                changed = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Y轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            if (ImGui::Button("Y", button_size))
            {
                values.y = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &values.y, 0.01f, -1.0f, 1.0f, "%.3f"))
                changed = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Z轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            if (ImGui::Button("Z", button_size))
            {
                values.z = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &values.z, 0.01f, -1.0f, 1.0f, "%.3f"))
                changed = true;
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // W轴
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.7f, 0.7f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.8f, 0.8f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.7f, 0.7f, 0.1f, 1.0f});
            if (ImGui::Button("W", button_size))
            {
                values.w = reset_value;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##W", &values.w, 0.01f, -1.0f, 1.0f, "%.3f"))
                changed = true;
            ImGui::PopItemWidth();
            
            ImGui::PopStyleVar();
            ImGui::Columns(1);
            ImGui::PopID();
            
            return changed;
        }
    }
}
