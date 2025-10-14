#include "editor/include/axis_calculator.h"
#include "runtime/core/math/math.h"
#include <cmath>

namespace Piccolo
{
    AxisCalculator::AxisCalculator(std::shared_ptr<RenderCamera> camera)
        : m_camera(std::move(camera))
    {
    }

    void AxisCalculator::updateCachedCameraParams() const
    {
        if (m_camera)
        {
            m_cached_fov = m_camera->getFovYDeprecated();
            m_cached_forward = m_camera->forward();
            m_cached_up = m_camera->up();
            m_cached_right = m_camera->right();
            m_cached_position = m_camera->position();
        }
    }

    Vector3 AxisCalculator::calculateWorldRayDirection(
        Vector2 cursor_uv,
        Vector2 window_size) const
    {
        updateCachedCameraParams();
        
        const float window_forward = window_size.y / 2.0f / Math::tan(Math::degreesToRadians(m_cached_fov) / 2.0f);
        const Vector2 screen_center_uv = Vector2(cursor_uv.x, 1 - cursor_uv.y) - Vector2(0.5f, 0.5f);
        
        return m_cached_forward * window_forward +
               m_cached_right * window_size.x * screen_center_uv.x +
               m_cached_up * window_size.y * screen_center_uv.y;
    }

    void AxisCalculator::calculateLocalRay(
        Vector3 world_ray_dir,
        Matrix4x4 model_matrix,
        Vector3& local_ray_origin,
        Vector3& local_ray_dir) const
    {
        updateCachedCameraParams();
        
        const Vector4 local_ray_origin_4 = model_matrix.inverse() * Vector4(m_cached_position, 1.0f);
        local_ray_origin = Vector3(local_ray_origin_4.x, local_ray_origin_4.y, local_ray_origin_4.z);
        
        Vector3 model_scale;
        Quaternion model_rotation;
        Vector3 model_translation;
        model_matrix.decomposition(model_translation, model_scale, model_rotation);
        
        Quaternion inversed_rotation = model_rotation.inverse();
        inversed_rotation.normalise();
        local_ray_dir = inversed_rotation * world_ray_dir;
    }

    float AxisCalculator::intersectPlaneRay(
        Vector3 normal, 
        float d, 
        Vector3 origin, 
        Vector3 dir)
    {
        const float deno = normal.dotProduct(dir);
        if (std::abs(deno) < AxisConstants::EPSILON)
        {
            return -(normal.dotProduct(origin) + d) / AxisConstants::EPSILON;
        }
        return -(normal.dotProduct(origin) + d) / deno;
    }

    AxisIntersectionResult AxisCalculator::calculateAxisIntersection(
        Vector2 cursor_uv,
        Vector2 window_size,
        Matrix4x4 model_matrix,
        int axis_mode) const
    {
        AxisIntersectionResult result;
        
        const Vector3 world_ray_dir = calculateWorldRayDirection(cursor_uv, window_size);
        Vector3 local_ray_origin, local_ray_dir;
        calculateLocalRay(world_ray_dir, model_matrix, local_ray_origin, local_ray_dir);
        
        // 计算与三个平面的交集点
        const Vector3 plane_normals[3] = {
            Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)
        };
        
        const float plane_view_depth[3] = {
            intersectPlaneRay(plane_normals[0], 0, local_ray_origin, local_ray_dir),
            intersectPlaneRay(plane_normals[1], 0, local_ray_origin, local_ray_dir),
            intersectPlaneRay(plane_normals[2], 0, local_ray_origin, local_ray_dir)
        };
        
        const Vector3 intersect_pts[3] = {
            local_ray_origin + plane_view_depth[0] * local_ray_dir, // yoz
            local_ray_origin + plane_view_depth[1] * local_ray_dir, // xoz
            local_ray_origin + plane_view_depth[2] * local_ray_dir  // xoy
        };
        
        if (axis_mode == 0 || axis_mode == 2) // translate or scale
        {
            result = calculateTranslateScaleIntersection(local_ray_dir, intersect_pts);
        }
        else if (axis_mode == 1) // rotate
        {
            result = calculateRotationIntersection(intersect_pts);
        }
        
        result.is_valid = (result.selected_axis != 3);
        return result;
    }

    AxisIntersectionResult AxisCalculator::calculateTranslateScaleIntersection(
        const Vector3& local_ray_dir,
        const Vector3 intersect_pts[3])
    {
        AxisIntersectionResult result;
        const Vector3 plane_normals[3] = {
            Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1)
        };
        
        float max_dist = 0.0f;
        
        // 检查射线是否在任意平面上
        for (int i = 0; i < 3; ++i)
        {
            const float local_ray_dir_proj = Math::abs(local_ray_dir.dotProduct(plane_normals[i]));
            const float cos_alpha = local_ray_dir_proj; // local_ray_dir is normalized
            
            if (cos_alpha <= AxisConstants::RAY_DIRECTION_THRESHOLD)
            {
                const int index00 = (i + 1) % 3;
                const int index01 = 3 - i - index00;
                const int index10 = (i + 2) % 3;
                const int index11 = 3 - i - index10;
                
                const float axis_dist = (Math::abs(intersect_pts[index00][i]) + Math::abs(intersect_pts[index10][i])) / 2;
                
                if (axis_dist > AxisConstants::DIST_THRESHOLD)
                {
                    continue;
                }
                
                // 检查哪个轴更近
                if ((intersect_pts[index00][index01] > AxisConstants::EDGE_OF_AXIS_MIN) &&
                    (intersect_pts[index00][index01] < AxisConstants::AXIS_LENGTH) &&
                    (intersect_pts[index00][index01] > max_dist) &&
                    (Math::abs(intersect_pts[index00][i]) < AxisConstants::EDGE_OF_AXIS_MAX))
                {
                    max_dist = intersect_pts[index00][index01];
                    result.selected_axis = index01;
                }
                
                if ((intersect_pts[index10][index11] > AxisConstants::EDGE_OF_AXIS_MIN) &&
                    (intersect_pts[index10][index11] < AxisConstants::AXIS_LENGTH) &&
                    (intersect_pts[index10][index11] > max_dist) &&
                    (Math::abs(intersect_pts[index10][i]) < AxisConstants::EDGE_OF_AXIS_MAX))
                {
                    max_dist = intersect_pts[index10][index11];
                    result.selected_axis = index11;
                }
            }
        }
        
        // 检查轴
        if (result.selected_axis == 3)
        {
            float min_dist = 1e10f;
            for (int i = 0; i < 3; ++i)
            {
                const int index0 = (i + 1) % 3;
                const int index1 = (i + 2) % 3;
                const float dist = Math::sqr(intersect_pts[index0][index1]) + Math::sqr(intersect_pts[index1][index0]);
                
                if ((intersect_pts[index0][i] > AxisConstants::EDGE_OF_AXIS_MIN) &&
                    (intersect_pts[index0][i] < AxisConstants::EDGE_OF_AXIS_MAX) &&
                    (dist < AxisConstants::DIST_THRESHOLD) &&
                    (dist < min_dist))
                {
                    min_dist = dist;
                    result.selected_axis = i;
                }
            }
        }
        
        result.distance = max_dist;
        return result;
    }

    AxisIntersectionResult AxisCalculator::calculateRotationIntersection(
        const Vector3 intersect_pts[3])
    {
        AxisIntersectionResult result;
        float min_dist = 1e10f;
        
        for (int i = 0; i < 3; ++i)
        {
            const float dist = std::abs(1 - std::hypot(intersect_pts[i].x, intersect_pts[i].y, intersect_pts[i].z));
            
            if ((dist < AxisConstants::ROTATION_DIST_THRESHOLD) && (dist < min_dist))
            {
                min_dist = dist;
                result.selected_axis = i;
            }
        }
        
        result.distance = min_dist;
        return result;
    }
}
