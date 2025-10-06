#include "editor/include/ui_components/transform_component_editor.h"
#include "editor/include/ui_components/ui_theme.h"
#include "runtime/core/math/math.h"
#include "runtime/core/math/matrix3.h"
#include <imgui.h>
#include <imgui_internal.h>

namespace Piccolo
{
    namespace Editor
    {
        TransformComponentEditor::TransformComponentEditor()
        {
            m_euler_angles = Vector3::ZERO;
        }

        void TransformComponentEditor::render()
        {
            if (!m_visible || !m_transform_component)
                return;

            ImGui::Begin("变换组件", &m_visible);
            
            // 渲染工具面板
            TransformToolPanel::renderTransformTools(this);
            ImGui::Separator();

            // 渲染变换控制
            renderPositionSection();
            renderRotationSection();
            renderScaleSection();
            
            ImGui::Separator();
            
            // 渲染变换工具
            if (ImGui::Button("重置到默认"))
                resetToDefault();
            
            ImGui::SameLine();
            if (ImGui::Button("复制变换"))
                copyTransform();
            
            ImGui::SameLine();
            if (ImGui::Button("粘贴变换"))
                pasteTransform();

            ImGui::End();
        }

        void TransformComponentEditor::setTransformComponent(std::shared_ptr<TransformComponent> transform)
        {
            m_transform_component = transform;
            if (transform)
            {
                // 更新欧拉角
                Quaternion rotation = transform->getRotation();
                m_euler_angles.x = rotation.getPitch(false).valueDegrees();
                m_euler_angles.y = rotation.getRoll(false).valueDegrees();
                m_euler_angles.z = rotation.getYaw(false).valueDegrees();
            }
        }

        void TransformComponentEditor::clearSelection()
        {
            m_transform_component = nullptr;
        }

        void TransformComponentEditor::renderPositionSection()
        {
            if (!m_transform_component)
                return;

            if (ImGui::CollapsingHeader("位置 (Position)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                Vector3 position = m_transform_component->getPosition();
                renderVector3Control("位置", position, 
                                   UITheme::Colors::Primary, 0.0f);
                m_transform_component->setPosition(position);
                
                ImGui::Unindent();
            }
        }

        void TransformComponentEditor::renderRotationSection()
        {
            if (!m_transform_component)
                return;

            if (ImGui::CollapsingHeader("旋转 (Rotation)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                
                // 旋转模式选择
                ImGui::Text("旋转模式:");
                ImGui::SameLine();
                if (ImGui::RadioButton("欧拉角", m_use_euler_angles))
                    m_use_euler_angles = true;
                ImGui::SameLine();
                if (ImGui::RadioButton("四元数", !m_use_euler_angles))
                    m_use_euler_angles = false;
                
                if (m_use_euler_angles)
                {
                    renderEulerAngles("欧拉角", m_euler_angles);
                }
                else
                {
                    Quaternion rotation = m_transform_component->getRotation();
                    renderRotationControl("四元数", rotation);
                    m_transform_component->setRotation(rotation);
                }
                
                ImGui::Unindent();
            }
        }

        void TransformComponentEditor::renderScaleSection()
        {
            if (!m_transform_component)
                return;

            if (ImGui::CollapsingHeader("缩放 (Scale)", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent();
                Vector3 scale = m_transform_component->getScale();
                renderVector3Control("缩放", scale, 
                                   UITheme::Colors::AxisW, 1.0f);
                m_transform_component->setScale(scale);
                
                // 统一缩放选项
                ImGui::Checkbox("统一缩放", nullptr);
                ImGui::SameLine();
                if (ImGui::Button("重置缩放"))
                    resetScale();
                
                ImGui::Unindent();
            }
        }

        void TransformComponentEditor::renderVector3Control(const std::string& label, Vector3& values, 
                                                          const ImVec4& color, float reset_value)
        {
            ImGui::PushID(label.c_str());
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("%s", label.c_str());
            ImGui::NextColumn();
            
            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
            
            float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImVec2 button_size = {line_height + 3.0f, line_height};
            
            // X轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisX, 
                                         UITheme::Colors::AxisX, 
                                         UITheme::Colors::AxisX);
            if (ImGui::Button("X", button_size))
            {
                values.x = reset_value;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.3f"))
            {
                // 值已更改
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Y轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisY, 
                                         UITheme::Colors::AxisY, 
                                         UITheme::Colors::AxisY);
            if (ImGui::Button("Y", button_size))
            {
                values.y = reset_value;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.3f"))
            {
                // 值已更改
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Z轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisZ, 
                                         UITheme::Colors::AxisZ, 
                                         UITheme::Colors::AxisZ);
            if (ImGui::Button("Z", button_size))
            {
                values.z = reset_value;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.3f"))
            {
                // 值已更改
            }
            ImGui::PopItemWidth();
            
            ImGui::PopStyleVar();
            ImGui::Columns(1);
            ImGui::PopID();
        }

        void TransformComponentEditor::renderRotationControl(const std::string& label, Quaternion& rotation)
        {
            ImGui::PushID(label.c_str());
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("%s", label.c_str());
            ImGui::NextColumn();
            
            ImGui::PushMultiItemsWidths(4, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
            
            float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImVec2 button_size = {line_height + 3.0f, line_height};
            
            // X轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisX, 
                                         UITheme::Colors::AxisX, 
                                         UITheme::Colors::AxisX);
            if (ImGui::Button("X", button_size))
            {
                rotation.x = 0.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &rotation.x, 0.01f, -1.0f, 1.0f, "%.3f"))
            {
                rotation.normalise();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Y轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisY, 
                                         UITheme::Colors::AxisY, 
                                         UITheme::Colors::AxisY);
            if (ImGui::Button("Y", button_size))
            {
                rotation.y = 0.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &rotation.y, 0.01f, -1.0f, 1.0f, "%.3f"))
            {
                rotation.normalise();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Z轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisZ, 
                                         UITheme::Colors::AxisZ, 
                                         UITheme::Colors::AxisZ);
            if (ImGui::Button("Z", button_size))
            {
                rotation.z = 0.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &rotation.z, 0.01f, -1.0f, 1.0f, "%.3f"))
            {
                rotation.normalise();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // W轴
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisW, 
                                         UITheme::Colors::AxisW, 
                                         UITheme::Colors::AxisW);
            if (ImGui::Button("W", button_size))
            {
                rotation.w = 1.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##W", &rotation.w, 0.01f, -1.0f, 1.0f, "%.3f"))
            {
                rotation.normalise();
            }
            ImGui::PopItemWidth();
            
            ImGui::PopStyleVar();
            ImGui::Columns(1);
            ImGui::PopID();
        }

        void TransformComponentEditor::renderEulerAngles(const std::string& label, Vector3& euler_angles)
        {
            ImGui::PushID(label.c_str());
            
            ImGui::Columns(2);
            ImGui::SetColumnWidth(0, 100.0f);
            ImGui::Text("%s", label.c_str());
            ImGui::NextColumn();
            
            ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});
            
            float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
            ImVec2 button_size = {line_height + 3.0f, line_height};
            
            // Pitch (X)
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisX, 
                                         UITheme::Colors::AxisX, 
                                         UITheme::Colors::AxisX);
            if (ImGui::Button("P", button_size))
            {
                euler_angles.x = 0.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Pitch", &euler_angles.x, 1.0f, -180.0f, 180.0f, "%.1f°"))
            {
                // 更新四元数
                if (m_transform_component)
                {
                    // 使用欧拉角创建四元数
                    float pitch = Math::degreesToRadians(euler_angles.x);
                    float yaw = Math::degreesToRadians(euler_angles.y);
                    float roll = Math::degreesToRadians(euler_angles.z);
                    
                    // 创建旋转四元数 (ZYX顺序)
                    Quaternion qx(Radian(pitch), Vector3::UNIT_X);
                    Quaternion qy(Radian(yaw), Vector3::UNIT_Y);
                    Quaternion qz(Radian(roll), Vector3::UNIT_Z);
                    
                    Quaternion new_rotation = qz * qy * qx;
                    m_transform_component->setRotation(new_rotation);
                }
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Yaw (Y)
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisY, 
                                         UITheme::Colors::AxisY, 
                                         UITheme::Colors::AxisY);
            if (ImGui::Button("Y", button_size))
            {
                euler_angles.y = 0.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Yaw", &euler_angles.y, 1.0f, -180.0f, 180.0f, "%.1f°"))
            {
                // 更新四元数
                if (m_transform_component)
                {
                    // 使用欧拉角创建四元数
                    float pitch = Math::degreesToRadians(euler_angles.x);
                    float yaw = Math::degreesToRadians(euler_angles.y);
                    float roll = Math::degreesToRadians(euler_angles.z);
                    
                    // 创建旋转四元数 (ZYX顺序)
                    Quaternion qx(Radian(pitch), Vector3::UNIT_X);
                    Quaternion qy(Radian(yaw), Vector3::UNIT_Y);
                    Quaternion qz(Radian(roll), Vector3::UNIT_Z);
                    
                    Quaternion new_rotation = qz * qy * qx;
                    m_transform_component->setRotation(new_rotation);
                }
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            
            // Roll (Z)
            UIStyleHelper::pushButtonStyle(UITheme::Colors::AxisZ, 
                                         UITheme::Colors::AxisZ, 
                                         UITheme::Colors::AxisZ);
            if (ImGui::Button("R", button_size))
            {
                euler_angles.z = 0.0f;
            }
            UIStyleHelper::popButtonStyle();
            
            ImGui::SameLine();
            if (ImGui::DragFloat("##Roll", &euler_angles.z, 1.0f, -180.0f, 180.0f, "%.1f°"))
            {
                // 更新四元数
                if (m_transform_component)
                {
                    // 使用欧拉角创建四元数
                    float pitch = Math::degreesToRadians(euler_angles.x);
                    float yaw = Math::degreesToRadians(euler_angles.y);
                    float roll = Math::degreesToRadians(euler_angles.z);
                    
                    // 创建旋转四元数 (ZYX顺序)
                    Quaternion qx(Radian(pitch), Vector3::UNIT_X);
                    Quaternion qy(Radian(yaw), Vector3::UNIT_Y);
                    Quaternion qz(Radian(roll), Vector3::UNIT_Z);
                    
                    Quaternion new_rotation = qz * qy * qx;
                    m_transform_component->setRotation(new_rotation);
                }
            }
            ImGui::PopItemWidth();
            
            ImGui::PopStyleVar();
            ImGui::Columns(1);
            ImGui::PopID();
        }

        void TransformComponentEditor::resetToDefault()
        {
            if (!m_transform_component)
                return;
                
            m_transform_component->setPosition(Vector3::ZERO);
            m_transform_component->setRotation(Quaternion::IDENTITY);
            m_transform_component->setScale(Vector3::UNIT_SCALE);
            m_euler_angles = Vector3::ZERO;
        }

        void TransformComponentEditor::copyTransform()
        {
            if (!m_transform_component)
                return;
                
            m_cached_transform.position = m_transform_component->getPosition();
            m_cached_transform.rotation = m_transform_component->getRotation();
            m_cached_transform.scale = m_transform_component->getScale();
        }

        void TransformComponentEditor::pasteTransform()
        {
            if (!m_transform_component)
                return;
                
            m_transform_component->setPosition(m_cached_transform.position);
            m_transform_component->setRotation(m_cached_transform.rotation);
            m_transform_component->setScale(m_cached_transform.scale);
            
            // 更新欧拉角
            Quaternion rotation = m_transform_component->getRotation();
            m_euler_angles.x = rotation.getPitch(false).valueDegrees();
            m_euler_angles.y = rotation.getRoll(false).valueDegrees();
            m_euler_angles.z = rotation.getYaw(false).valueDegrees();
        }

        void TransformComponentEditor::resetPosition()
        {
            if (m_transform_component)
                m_transform_component->setPosition(Vector3::ZERO);
        }

        void TransformComponentEditor::resetRotation()
        {
            if (m_transform_component)
            {
                m_transform_component->setRotation(Quaternion::IDENTITY);
                m_euler_angles = Vector3::ZERO;
            }
        }

        void TransformComponentEditor::resetScale()
        {
            if (m_transform_component)
                m_transform_component->setScale(Vector3::UNIT_SCALE);
        }

        // Transform工具面板实现
        void TransformToolPanel::renderTransformTools(TransformComponentEditor* editor)
        {
            if (ImGui::Button("重置位置"))
                editor->resetPosition();
            ImGui::SameLine();
            if (ImGui::Button("重置旋转"))
                editor->resetRotation();
            ImGui::SameLine();
            if (ImGui::Button("重置缩放"))
                editor->resetScale();
        }

        void TransformToolPanel::renderSnapSettings()
        {
            ImGui::Text("捕捉设置");
            static float position_snap = 1.0f;
            static float rotation_snap = 15.0f;
            static float scale_snap = 0.1f;
            
            ImGui::InputFloat("位置捕捉", &position_snap, 0.1f, 1.0f, "%.1f");
            ImGui::InputFloat("旋转捕捉", &rotation_snap, 1.0f, 5.0f, "%.1f°");
            ImGui::InputFloat("缩放捕捉", &scale_snap, 0.01f, 0.1f, "%.2f");
        }

        void TransformToolPanel::renderGizmoSettings()
        {
            ImGui::Text("Gizmo设置");
            static bool show_gizmo = true;
            static int gizmo_mode = 0;
            
            ImGui::Checkbox("显示Gizmo", &show_gizmo);
            ImGui::RadioButton("本地", &gizmo_mode, 0);
            ImGui::SameLine();
            ImGui::RadioButton("世界", &gizmo_mode, 1);
        }
    }
}
