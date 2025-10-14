#pragma once

#include "editor/include/axis_calculator.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/core/math/vector2.h"
#include "runtime/core/math/matrix4.h"

namespace Piccolo
{
    // 轴操作类型
    enum class AxisOperationType
    {
        Translate,
        Rotate,
        Scale
    };

    // 轴操作参数
    struct AxisOperationParams
    {
        Vector2 new_mouse_pos;
        Vector2 last_mouse_pos;
        Vector2 engine_window_pos;
        Vector2 engine_window_size;
        size_t cursor_on_axis;
        Matrix4x4 model_matrix;
        AxisOperationType operation_type;
    };

    // 轴操作结果
    struct AxisOperationResult
    {
        bool success = false;
        Vector3 new_translation;
        Quaternion new_rotation;
        Vector3 new_scale;
        Matrix4x4 new_model_matrix;
    };

    // 轴操作管理器
    class AxisOperationManager
    {
    public:
        explicit AxisOperationManager(std::shared_ptr<RenderCamera> camera);
        
        // 执行轴操作
        AxisOperationResult performAxisOperation(
            const AxisOperationParams& params,
            TransformComponent* transform_component) const;

    private:
        std::unique_ptr<AxisCalculator> m_axis_calculator;
        
        // 计算角速度
        static float calculateAngularVelocity(Vector2 window_size);
        
        // 执行平移操作
        AxisOperationResult performTranslateOperation(
            const AxisOperationParams& params,
            const TransformComponent* transform_component) const;
            
        // 执行旋转操作
        AxisOperationResult performRotateOperation(
            const AxisOperationParams& params,
            const TransformComponent* transform_component) const;
            
        // 执行缩放操作
        AxisOperationResult performScaleOperation(
            const AxisOperationParams& params,
            const TransformComponent* transform_component) const;
            
        // 计算轴方向向量
        static Vector2 calculateAxisDirection(
            Vector3 local_axis_position,
            Matrix4x4 axis_model_matrix,
            Matrix4x4 view_matrix,
            Matrix4x4 proj_matrix,
            Vector3 model_translation);
    };
}
