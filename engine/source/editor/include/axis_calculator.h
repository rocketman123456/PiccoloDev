#pragma once

#include "runtime/function/render/render_camera.h"
#include "runtime/core/math/vector3.h"
#include "runtime/core/math/vector2.h"
#include "runtime/core/math/matrix4.h"
#include "runtime/core/math/quaternion.h"

namespace Piccolo
{
    // 轴计算常量
    namespace AxisConstants
    {
        constexpr float DIST_THRESHOLD = 0.6f;
        constexpr float EDGE_OF_AXIS_MIN = 0.1f;
        constexpr float EDGE_OF_AXIS_MAX = 2.0f;
        constexpr float AXIS_LENGTH = 2.0f;
        constexpr float ROTATION_DIST_THRESHOLD = 0.2f;
        constexpr float RAY_DIRECTION_THRESHOLD = 0.15f; // cos(80deg)
        constexpr float EPSILON = 0.0001f;
        constexpr float ANGULAR_VELOCITY_BASE = 18.0f; // degrees
    }

    // 轴交集结果
    struct AxisIntersectionResult
    {
        size_t selected_axis = 3; // 3 means no axis selected
        float distance = 0.0f;
        bool is_valid = false;
    };

    // 轴计算器类
    class AxisCalculator
    {
    public:
        explicit AxisCalculator(std::shared_ptr<RenderCamera> camera);
        
        // 计算鼠标与轴的交集
        AxisIntersectionResult calculateAxisIntersection(
            Vector2 cursor_uv,
            Vector2 window_size,
            Matrix4x4 model_matrix,
            int axis_mode) const;

        // 计算射线与平面的交集
        static float intersectPlaneRay(
            Vector3 normal, 
            float d, 
            Vector3 origin, 
            Vector3 dir);

        // 计算世界射线方向
        Vector3 calculateWorldRayDirection(
            Vector2 cursor_uv,
            Vector2 window_size) const;

        // 计算局部射线
        void calculateLocalRay(
            Vector3 world_ray_dir,
            Matrix4x4 model_matrix,
            Vector3& local_ray_origin,
            Vector3& local_ray_dir) const;

        // 获取相机指针（用于轴操作管理器）
        std::shared_ptr<RenderCamera> getCamera() const { return m_camera; }

    private:
        std::shared_ptr<RenderCamera> m_camera;
        
        // 缓存的相机参数
        mutable float m_cached_fov = -1.0f;
        mutable Vector3 m_cached_forward;
        mutable Vector3 m_cached_up;
        mutable Vector3 m_cached_right;
        mutable Vector3 m_cached_position;
        
        // 更新缓存的相机参数
        void updateCachedCameraParams() const;
        
        // 计算平移/缩放轴交集
        static AxisIntersectionResult calculateTranslateScaleIntersection(
            const Vector3& local_ray_dir,
            const Vector3 intersect_pts[3]);
            
        // 计算旋转轴交集
        static AxisIntersectionResult calculateRotationIntersection(
            const Vector3 intersect_pts[3]);
    };
}
