#pragma once

#include "runtime/core/math/quaternion.h"
#include "runtime/core/math/vector3.h"
#include "runtime/function/framework/component/transform/transform_component.h"

#include "editor/include/ui_components/ui_component_base.h"

#include <imgui.h>

namespace Piccolo
{
    namespace Editor
    {
        // Transform组件专用编辑器
        class TransformComponentEditor : public UIComponentBase
        {
        public:
            TransformComponentEditor();
            ~TransformComponentEditor() override = default;

            void render() override;
            void setTransformComponent(std::shared_ptr<TransformComponent> transform);
            void clearSelection();

            // 工具方法
            void resetToDefault();
            void copyTransform();
            void pasteTransform();
            void resetPosition();
            void resetRotation();
            void resetScale();

        private:
            void renderPositionSection();
            void renderRotationSection();
            void renderScaleSection();
            void renderTransformGizmo();

            // 辅助方法
            void renderVector3Control(const std::string& label, Vector3& values, const ImVec4& color, float reset_value = 0.0f);
            void renderRotationControl(const std::string& label, Quaternion& rotation);
            void renderEulerAngles(const std::string& label, Vector3& euler_angles);

            std::shared_ptr<TransformComponent> m_transform_component;

            // 内部状态
            Vector3 m_euler_angles;
            bool    m_use_euler_angles = true;
            bool    m_show_gizmo       = true;

            // 缓存的变换数据（用于复制粘贴）
            struct CachedTransform
            {
                Vector3    position;
                Quaternion rotation;
                Vector3    scale;
            } m_cached_transform;
        };

        // Transform工具面板
        class TransformToolPanel
        {
        public:
            static void renderTransformTools(TransformComponentEditor* editor);
            static void renderSnapSettings();
            static void renderGizmoSettings();
        };
    } // namespace Editor
} // namespace Piccolo
