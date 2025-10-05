#pragma once

#include "runtime/function/render/render_entity.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_type.h"

#include <array>
#include <memory>
#include <optional>

namespace Piccolo
{
    // 前向声明
    class WindowSystem;
    class RHI;
    class RenderResourceBase;
    class RenderPipelineBase;
    class RenderScene;
    class RenderCamera;
    class WindowUI;
    class DebugDrawManager;

    /**
     * @brief 渲染系统初始化信息
     * 包含渲染系统初始化所需的所有依赖组件
     */
    struct RenderSystemInitInfo
    {
        std::shared_ptr<WindowSystem>     window_system;      ///< 窗口系统
        std::shared_ptr<DebugDrawManager> debugdraw_manager;  ///< 调试绘制管理器
    };

    /**
     * @brief 引擎内容视口信息
     * 定义渲染区域的位置和大小
     */
    struct EngineContentViewport
    {
        float x {0.f};      ///< 视口X坐标
        float y {0.f};      ///< 视口Y坐标
        float width {0.f};  ///< 视口宽度
        float height {0.f}; ///< 视口高度
    };

    /**
     * @brief 渲染系统
     * 
     * 负责管理整个渲染流程的核心类，包括：
     * - 渲染硬件接口(RHI)的管理
     * - 渲染管线的创建和执行
     * - 渲染资源的分配和管理
     * - 渲染场景和相机的管理
     * - 逻辑线程和渲染线程之间的数据交换
     * 
     * 支持前向渲染和延迟渲染两种管线类型
     */
    class RenderSystem
    {
    public:
        RenderSystem() = default;
        ~RenderSystem();

        // ========== 生命周期管理 ==========
        
        /**
         * @brief 初始化渲染系统
         * @param init_info 初始化信息
         */
        void initialize(RenderSystemInitInfo init_info);
        
        /**
         * @brief 每帧更新和渲染
         * @param delta_time 帧间隔时间
         */
        void tick(float delta_time);
        
        /**
         * @brief 清理渲染系统资源
         */
        void clear();

        // ========== 数据交换接口 ==========
        
        /**
         * @brief 交换逻辑和渲染线程间的数据
         */
        void swapLogicRenderData();
        
        /**
         * @brief 获取渲染交换上下文
         * @return 渲染交换上下文的引用
         */
        RenderSwapContext& getSwapContext();

        // ========== 组件访问接口 ==========
        
        /**
         * @brief 获取渲染相机
         * @return 渲染相机的智能指针
         */
        std::shared_ptr<RenderCamera> getRenderCamera() const;
        
        /**
         * @brief 获取渲染硬件接口
         * @return RHI的智能指针
         */
        std::shared_ptr<RHI> getRHI() const;

        // ========== 渲染配置接口 ==========
        
        /**
         * @brief 设置渲染管线类型
         * @param pipeline_type 管线类型（前向/延迟）
         */
        void setRenderPipelineType(RENDER_PIPELINE_TYPE pipeline_type);
        
        /**
         * @brief 初始化UI渲染后端
         * @param window_ui UI窗口对象
         */
        void initializeUIRenderBackend(WindowUI* window_ui);
        
        /**
         * @brief 更新引擎内容视口
         * @param offset_x X偏移
         * @param offset_y Y偏移
         * @param width 宽度
         * @param height 高度
         */
        void updateEngineContentViewport(float offset_x, float offset_y, float width, float height);

        // ========== 拾取和选择接口 ==========
        
        /**
         * @brief 根据拾取UV坐标获取网格GUID
         * @param picked_uv 拾取的UV坐标
         * @return 网格的GUID
         */
        uint32_t getGuidOfPickedMesh(const Vector2& picked_uv);
        
        /**
         * @brief 根据网格ID获取游戏对象ID
         * @param mesh_id 网格ID
         * @return 游戏对象ID
         */
        GObjectID getGObjectIDByMeshID(uint32_t mesh_id) const;

        // ========== 视口管理接口 ==========
        
        /**
         * @brief 获取引擎内容视口
         * @return 视口信息
         */
        EngineContentViewport getEngineContentViewport() const;

        // ========== 坐标轴管理接口 ==========
        
        /**
         * @brief 创建坐标轴
         * @param axis_entities 坐标轴实体数组
         * @param mesh_datas 网格数据数组
         */
        void createAxis(std::array<RenderEntity, 3> axis_entities, std::array<RenderMeshData, 3> mesh_datas);
        
        /**
         * @brief 设置可见的坐标轴
         * @param axis 坐标轴实体（可选）
         */
        void setVisibleAxis(std::optional<RenderEntity> axis);
        
        /**
         * @brief 设置选中的坐标轴
         * @param selected_axis 选中的坐标轴索引
         */
        void setSelectedAxis(size_t selected_axis);

        // ========== 资源分配器接口 ==========
        
        /**
         * @brief 获取游戏对象实例ID分配器
         * @return 游戏对象实例ID分配器的引用
         */
        GuidAllocator<GameObjectPartId>& getGOInstanceIdAllocator();
        
        /**
         * @brief 获取网格资源ID分配器
         * @return 网格资源ID分配器的引用
         */
        GuidAllocator<MeshSourceDesc>& getMeshAssetIdAllocator();

        // ========== 场景管理接口 ==========
        
        /**
         * @brief 清理场景重新加载
         */
        void clearForLevelReloading();

    private:
        // ========== 私有成员变量 ==========
        
        RENDER_PIPELINE_TYPE m_render_pipeline_type {RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE}; ///< 渲染管线类型

        RenderSwapContext m_swap_context; ///< 渲染交换上下文

        // 核心渲染组件
        std::shared_ptr<RHI>                m_rhi;           ///< 渲染硬件接口
        std::shared_ptr<RenderCamera>       m_render_camera; ///< 渲染相机
        std::shared_ptr<RenderScene>        m_render_scene;  ///< 渲染场景
        std::shared_ptr<RenderResourceBase> m_render_resource; ///< 渲染资源
        std::shared_ptr<RenderPipelineBase> m_render_pipeline; ///< 渲染管线

        // ========== 私有方法 ==========
        
        /**
         * @brief 处理逻辑和渲染线程间的数据交换
         */
        void processSwapData();
    };
} // namespace Piccolo
