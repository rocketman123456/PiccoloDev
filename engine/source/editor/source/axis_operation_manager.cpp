#include "editor/include/axis_operation_manager.h"
#include "runtime/core/math/math.h"

namespace Piccolo
{
    AxisOperationManager::AxisOperationManager(std::shared_ptr<RenderCamera> camera)
        : m_axis_calculator(std::make_unique<AxisCalculator>(camera))
    {
    }

    float AxisOperationManager::calculateAngularVelocity(Vector2 window_size)
    {
        return AxisConstants::ANGULAR_VELOCITY_BASE / Math::max(window_size.x, window_size.y);
    }

    Vector2 AxisOperationManager::calculateAxisDirection(
        Vector3 local_axis_position,
        Matrix4x4 axis_model_matrix,
        Matrix4x4 view_matrix,
        Matrix4x4 proj_matrix,
        Vector3 model_translation)
    {
        Vector4 axis_world_position_4 = axis_model_matrix * Vector4(local_axis_position, 1.0f);
        axis_world_position_4.w = 1.0f;
        
        Vector4 axis_clip_position = proj_matrix * view_matrix * axis_world_position_4;
        axis_clip_position /= axis_clip_position.w;
        
        Vector2 axis_clip_uv((axis_clip_position.x + 1) / 2.0f, (axis_clip_position.y + 1) / 2.0f);
        
        Vector4 model_world_position_4(model_translation, 1.0f);
        Vector4 model_origin_clip_position = proj_matrix * view_matrix * model_world_position_4;
        model_origin_clip_position /= model_origin_clip_position.w;
        
        Vector2 model_origin_clip_uv(
            (model_origin_clip_position.x + 1) / 2.0f,
            (model_origin_clip_position.y + 1) / 2.0f
        );
        
        Vector2 axis_direction_uv = axis_clip_uv - model_origin_clip_uv;
        axis_direction_uv.normalise();
        
        return axis_direction_uv;
    }

    AxisOperationResult AxisOperationManager::performAxisOperation(
        const AxisOperationParams& params,
        TransformComponent* transform_component) const
    {
        if (!transform_component)
        {
            return {};
        }

        switch (params.operation_type)
        {
            case AxisOperationType::Translate:
                return performTranslateOperation(params, transform_component);
            case AxisOperationType::Rotate:
                return performRotateOperation(params, transform_component);
            case AxisOperationType::Scale:
                return performScaleOperation(params, transform_component);
            default:
                return {};
        }
    }

    AxisOperationResult AxisOperationManager::performTranslateOperation(
        const AxisOperationParams& params,
        const TransformComponent* /*transform_component*/) const
    {
        AxisOperationResult result;
        
        const float angular_velocity = calculateAngularVelocity(params.engine_window_size);
        const Vector2 delta_mouse_move_uv = {
            params.new_mouse_pos.x - params.last_mouse_pos.x,
            params.new_mouse_pos.y - params.last_mouse_pos.y
        };

        Vector3 model_scale, model_translation;
        Quaternion model_rotation;
        params.model_matrix.decomposition(model_translation, model_scale, model_rotation);

        Matrix4x4 axis_model_matrix = Matrix4x4::IDENTITY;
        axis_model_matrix.setTrans(model_translation);

        Matrix4x4 view_matrix = m_axis_calculator->getCamera()->getLookAtMatrix();
        Matrix4x4 proj_matrix = m_axis_calculator->getCamera()->getPersProjMatrix();

        // 计算轴方向向量
        Vector2 axis_directions[3];
        Vector3 axis_positions[3] = {Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)};
        
        for (int i = 0; i < 3; ++i)
        {
            axis_directions[i] = calculateAxisDirection(
                axis_positions[i], axis_model_matrix, view_matrix, proj_matrix, model_translation);
        }

        Vector3 move_vector = Vector3::ZERO;
        if (params.cursor_on_axis < 3)
        {
            move_vector[params.cursor_on_axis] = 
                delta_mouse_move_uv.dotProduct(axis_directions[params.cursor_on_axis]) * angular_velocity;
        }
        else
        {
            return result; // 无效的轴选择
        }

        Matrix4x4 translate_mat;
        translate_mat.makeTransform(move_vector, Vector3::UNIT_SCALE, Quaternion::IDENTITY);
        
        Matrix4x4 new_model_matrix = axis_model_matrix * translate_mat;
        new_model_matrix = new_model_matrix * Matrix4x4(model_rotation);
        new_model_matrix = new_model_matrix * Matrix4x4::buildScaleMatrix(model_scale.x, model_scale.y, model_scale.z);

        new_model_matrix.decomposition(result.new_translation, result.new_scale, result.new_rotation);
        result.new_model_matrix = new_model_matrix;
        result.success = true;

        return result;
    }

    AxisOperationResult AxisOperationManager::performRotateOperation(
        const AxisOperationParams& params,
        const TransformComponent* /*transform_component*/) const
    {
        AxisOperationResult result;
        
        const float angular_velocity = calculateAngularVelocity(params.engine_window_size);
        const Vector2 delta_mouse_move_uv = {
            params.new_mouse_pos.x - params.last_mouse_pos.x,
            params.new_mouse_pos.y - params.last_mouse_pos.y
        };

        Vector3 model_scale, model_translation;
        Quaternion model_rotation;
        params.model_matrix.decomposition(model_translation, model_scale, model_rotation);

        Matrix4x4 axis_model_matrix = Matrix4x4::IDENTITY;
        axis_model_matrix.setTrans(model_translation);

        Matrix4x4 view_matrix = m_axis_calculator->getCamera()->getLookAtMatrix();
        Matrix4x4 proj_matrix = m_axis_calculator->getCamera()->getPersProjMatrix();

        Vector4 model_world_position_4(model_translation, 1.0f);
        Vector4 model_origin_clip_position = proj_matrix * view_matrix * model_world_position_4;
        model_origin_clip_position /= model_origin_clip_position.w;
        
        Vector2 model_origin_clip_uv(
            (model_origin_clip_position.x + 1) / 2.0f,
            (model_origin_clip_position.y + 1) / 2.0f
        );

        const float last_mouse_u = (params.last_mouse_pos.x - params.engine_window_pos.x) / params.engine_window_size.x;
        const float last_mouse_v = (params.last_mouse_pos.y - params.engine_window_pos.y) / params.engine_window_size.y;
        const Vector2 last_move_vector(last_mouse_u - model_origin_clip_uv.x, last_mouse_v - model_origin_clip_uv.y);
        
        const float new_mouse_u = (params.new_mouse_pos.x - params.engine_window_pos.x) / params.engine_window_size.x;
        const float new_mouse_v = (params.new_mouse_pos.y - params.engine_window_pos.y) / params.engine_window_size.y;
        const Vector2 new_move_vector(new_mouse_u - model_origin_clip_uv.x, new_mouse_v - model_origin_clip_uv.y);

        float move_radian;
        Vector3 axis_of_rotation = Vector3::ZERO;
        
        if (params.cursor_on_axis < 3)
        {
            move_radian = (delta_mouse_move_uv * angular_velocity).length();
            
            // 根据相机方向确定旋转方向
            const Vector3 camera_forward = m_axis_calculator->getCamera()->forward();
            const Vector3 unit_axes[3] = {Vector3::UNIT_X, Vector3::UNIT_Y, Vector3::UNIT_Z};
            
            if (camera_forward.dotProduct(unit_axes[params.cursor_on_axis]) < 0)
            {
                move_radian = -move_radian;
            }
            
            axis_of_rotation[params.cursor_on_axis] = 1;
        }
        else
        {
            return result; // 无效的轴选择
        }

        // 确定旋转方向
        const float move_direction = last_move_vector.x * new_move_vector.y - new_move_vector.x * last_move_vector.y;
        if (move_direction < 0)
        {
            move_radian = -move_radian;
        }

        Quaternion move_rot;
        move_rot.fromAngleAxis(Radian(move_radian), axis_of_rotation);
        
        Matrix4x4 new_model_matrix = axis_model_matrix * move_rot;
        new_model_matrix = new_model_matrix * Matrix4x4(model_rotation);
        new_model_matrix = new_model_matrix * Matrix4x4::buildScaleMatrix(model_scale.x, model_scale.y, model_scale.z);

        new_model_matrix.decomposition(result.new_translation, result.new_scale, result.new_rotation);
        result.new_model_matrix = new_model_matrix;
        result.success = true;

        return result;
    }

    AxisOperationResult AxisOperationManager::performScaleOperation(
        const AxisOperationParams& params,
        const TransformComponent* /*transform_component*/) const
    {
        AxisOperationResult result;
        
        const Vector2 delta_mouse_move_uv = {
            params.new_mouse_pos.x - params.last_mouse_pos.x,
            params.new_mouse_pos.y - params.last_mouse_pos.y
        };

        Vector3 model_scale, model_translation;
        Quaternion model_rotation;
        params.model_matrix.decomposition(model_translation, model_scale, model_rotation);

        Matrix4x4 axis_model_matrix = Matrix4x4::IDENTITY;
        axis_model_matrix.setTrans(model_translation);

        Matrix4x4 view_matrix = m_axis_calculator->getCamera()->getLookAtMatrix();
        Matrix4x4 proj_matrix = m_axis_calculator->getCamera()->getPersProjMatrix();

        // 计算轴方向向量
        Vector2 axis_directions[3];
        Vector3 axis_positions[3] = {Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)};
        
        for (int i = 0; i < 3; ++i)
        {
            Vector3 rotated_axis_position = Matrix4x4(model_rotation) * axis_positions[i];
            axis_directions[i] = calculateAxisDirection(
                rotated_axis_position, axis_model_matrix, view_matrix, proj_matrix, model_translation);
        }

        Vector3 delta_scale_vector = Vector3::ZERO;
        if (params.cursor_on_axis < 3)
        {
            delta_scale_vector[params.cursor_on_axis] = 0.01f;
            if (delta_mouse_move_uv.dotProduct(axis_directions[params.cursor_on_axis]) < 0)
            {
                delta_scale_vector[params.cursor_on_axis] = -0.01f;
            }
        }
        else
        {
            return result; // 无效的轴选择
        }

        result.new_scale = model_scale + delta_scale_vector;
        axis_model_matrix = axis_model_matrix * Matrix4x4(model_rotation);
        
        Matrix4x4 scale_mat;
        scale_mat.makeTransform(Vector3::ZERO, result.new_scale, Quaternion::IDENTITY);
        
        result.new_model_matrix = axis_model_matrix * scale_mat;
        result.new_model_matrix.decomposition(result.new_translation, result.new_scale, result.new_rotation);
        result.success = true;

        return result;
    }
}
