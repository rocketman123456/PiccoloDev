#pragma once

#include "runtime/core/math/quaternion.h"
#include "runtime/core/math/vector3.h"

#include "editor/include/ui_components/ui_component_base.h"

#include <string>

namespace Piccolo
{
    namespace Editor
    {
        // 布尔值编辑器
        class BoolPropertyEditor : public PropertyEditor<bool>
        {
        protected:
            void        renderImpl(const std::string& label, bool* value) override;
            std::string getTypeName() const override { return "bool"; }
        };

        // 整数编辑器
        class IntPropertyEditor : public PropertyEditor<int>
        {
        protected:
            void        renderImpl(const std::string& label, int* value) override;
            std::string getTypeName() const override { return "int"; }
        };

        // 浮点数编辑器
        class FloatPropertyEditor : public PropertyEditor<float>
        {
        protected:
            void        renderImpl(const std::string& label, float* value) override;
            std::string getTypeName() const override { return "float"; }
        };

        // 字符串编辑器
        class StringPropertyEditor : public PropertyEditor<std::string>
        {
        protected:
            void        renderImpl(const std::string& label, std::string* value) override;
            std::string getTypeName() const override { return "std::string"; }
        };

        // Vector3编辑器
        class Vector3PropertyEditor : public PropertyEditor<Vector3>
        {
        public:
            Vector3PropertyEditor(float reset_value = 0.0f, float column_width = 100.0f);

        protected:
            void        renderImpl(const std::string& label, Vector3* value) override;
            std::string getTypeName() const override { return "Vector3"; }

        private:
            float m_reset_value;
            float m_column_width;
        };

        // Quaternion编辑器
        class QuaternionPropertyEditor : public PropertyEditor<Quaternion>
        {
        public:
            QuaternionPropertyEditor(float reset_value = 0.0f, float column_width = 100.0f);

        protected:
            void        renderImpl(const std::string& label, Quaternion* value) override;
            std::string getTypeName() const override { return "Quaternion"; }

        private:
            float m_reset_value;
            float m_column_width;
        };

        // 通用拖拽控件
        class DragControl
        {
        public:
            static bool drawVec3Control(const std::string& label, Vector3& values, float reset_value = 0.0f, float column_width = 100.0f);
            static bool drawQuatControl(const std::string& label, Quaternion& values, float reset_value = 0.0f, float column_width = 100.0f);
        };
    } // namespace Editor
} // namespace Piccolo
