#pragma once

#include "runtime/core/math/vector2.h"
#include "runtime/function/render/render_pass_base.h"
#include "runtime/function/render/render_type.h"

#include <memory>
#include <vector>
#include <string>

namespace Piccolo
{
    // 前向声明
    class RHI;
    class RenderResourceBase;
    class WindowUI;

    /**
     * @brief 渲染管线初始化信息
     * 包含渲染管线初始化所需的所有配置
     */
    struct RenderPipelineInitInfo
    {
        bool                                enable_fxaa {false};        ///< 是否启用FXAA抗锯齿
        std::shared_ptr<RenderResourceBase> render_resource;           ///< 渲染资源
        RENDER_PIPELINE_TYPE                pipeline_type {RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE}; ///< 管线类型
    };

    /**
     * @brief 渲染管线状态
     */
    enum class RenderPipelineState : uint8_t
    {
        UNINITIALIZED = 0,    ///< 未初始化
        INITIALIZING,         ///< 初始化中
        INITIALIZED,          ///< 已初始化
        RENDERING,            ///< 渲染中
        ERROR                 ///< 错误状态
    };

    /**
     * @brief 渲染管线基类
     * 
     * 提供渲染管线的基础功能，包括：
     * - 渲染通道的管理和调度
     * - 前向渲染和延迟渲染的实现
     * - 渲染状态的跟踪和管理
     * - 错误处理和恢复机制
     * 
     * 支持多种渲染管线类型，包括前向渲染和延迟渲染
     */
    class RenderPipelineBase
    {
        friend class RenderSystem;

    public:
        /**
         * @brief 构造函数
         */
        RenderPipelineBase() = default;

        /**
         * @brief 析构函数
         */
        virtual ~RenderPipelineBase() = default;

        // ========== 生命周期管理 ==========

        /**
         * @brief 清理渲染管线资源
         */
        virtual void clear();

        /**
         * @brief 初始化渲染管线
         * @param init_info 初始化信息
         * @return 是否初始化成功
         */
        virtual bool initialize(RenderPipelineInitInfo init_info) = 0;

        // ========== 渲染执行接口 ==========

        /**
         * @brief 准备渲染通道数据
         * @param render_resource 渲染资源
         * @return 是否准备成功
         */
        virtual bool preparePassData(std::shared_ptr<RenderResourceBase> render_resource);

        /**
         * @brief 执行前向渲染
         * @param rhi 渲染硬件接口
         * @param render_resource 渲染资源
         * @return 是否渲染成功
         */
        virtual bool forwardRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource);

        /**
         * @brief 执行延迟渲染
         * @param rhi 渲染硬件接口
         * @param render_resource 渲染资源
         * @return 是否渲染成功
         */
        virtual bool deferredRender(std::shared_ptr<RHI> rhi, std::shared_ptr<RenderResourceBase> render_resource);

        // ========== UI和拾取接口 ==========

        /**
         * @brief 初始化UI渲染后端
         * @param window_ui UI窗口对象
         * @return 是否初始化成功
         */
        bool initializeUIRenderBackend(WindowUI* window_ui);

        /**
         * @brief 根据拾取UV坐标获取网格GUID
         * @param picked_uv 拾取的UV坐标
         * @return 网格的GUID
         */
        virtual uint32_t getGuidOfPickedMesh(const Vector2& picked_uv) = 0;

        // ========== 状态管理接口 ==========

        /**
         * @brief 获取当前管线状态
         * @return 管线状态
         */
        RenderPipelineState getState() const { return m_state; }

        /**
         * @brief 检查管线是否已初始化
         * @return 是否已初始化
         */
        bool isInitialized() const { return m_state == RenderPipelineState::INITIALIZED; }

        /**
         * @brief 检查管线是否处于错误状态
         * @return 是否处于错误状态
         */
        bool hasError() const { return m_state == RenderPipelineState::ERROR; }

        /**
         * @brief 获取最后的错误消息
         * @return 错误消息
         */
        const std::string& getLastError() const { return m_last_error; }

        /**
         * @brief 重置错误状态
         */
        void resetError();

        // ========== 渲染通道访问接口 ==========

        /**
         * @brief 获取方向光阴影通道
         * @return 方向光阴影通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getDirectionalLightPass() const { return m_directional_light_pass; }

        /**
         * @brief 获取点光源阴影通道
         * @return 点光源阴影通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getPointLightShadowPass() const { return m_point_light_shadow_pass; }

        /**
         * @brief 获取主相机通道
         * @return 主相机通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getMainCameraPass() const { return m_main_camera_pass; }

        /**
         * @brief 获取颜色分级通道
         * @return 颜色分级通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getColorGradingPass() const { return m_color_grading_pass; }

        /**
         * @brief 获取暗角效果通道
         * @return 暗角效果通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getVignettePass() const { return m_vignette_pass; }

        /**
         * @brief 获取FXAA抗锯齿通道
         * @return FXAA抗锯齿通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getFXAAPass() const { return m_fxaa_pass; }

        /**
         * @brief 获取色调映射通道
         * @return 色调映射通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getToneMappingPass() const { return m_tone_mapping_pass; }

        /**
         * @brief 获取UI渲染通道
         * @return UI渲染通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getUIPass() const { return m_ui_pass; }

        /**
         * @brief 获取UI合成通道
         * @return UI合成通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getCombineUIPass() const { return m_combine_ui_pass; }

        /**
         * @brief 获取拾取通道
         * @return 拾取通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getPickPass() const { return m_pick_pass; }

        /**
         * @brief 获取粒子系统通道
         * @return 粒子系统通道的智能指针
         */
        std::shared_ptr<RenderPassBase> getParticlePass() const { return m_particle_pass; }

    protected:
        // ========== 受保护的成员变量 ==========

        std::shared_ptr<RHI> m_rhi; ///< 渲染硬件接口

        // 渲染通道组件
        std::shared_ptr<RenderPassBase> m_directional_light_pass;  ///< 方向光阴影通道
        std::shared_ptr<RenderPassBase> m_point_light_shadow_pass; ///< 点光源阴影通道
        std::shared_ptr<RenderPassBase> m_main_camera_pass;        ///< 主相机通道
        std::shared_ptr<RenderPassBase> m_color_grading_pass;      ///< 颜色分级通道
        std::shared_ptr<RenderPassBase> m_vignette_pass;           ///< 暗角效果通道
        std::shared_ptr<RenderPassBase> m_fxaa_pass;               ///< FXAA抗锯齿通道
        std::shared_ptr<RenderPassBase> m_tone_mapping_pass;       ///< 色调映射通道
        std::shared_ptr<RenderPassBase> m_ui_pass;                 ///< UI渲染通道
        std::shared_ptr<RenderPassBase> m_combine_ui_pass;         ///< UI合成通道
        std::shared_ptr<RenderPassBase> m_pick_pass;               ///< 拾取通道
        std::shared_ptr<RenderPassBase> m_particle_pass;           ///< 粒子系统通道

        // 状态管理
        RenderPipelineState m_state = RenderPipelineState::UNINITIALIZED; ///< 当前状态
        std::string m_last_error;                                        ///< 最后的错误消息

        // ========== 受保护的方法 ==========

        /**
         * @brief 设置错误状态
         * @param error_message 错误消息
         */
        void setError(const std::string& error_message);

        /**
         * @brief 设置管线状态
         * @param state 新状态
         */
        void setState(RenderPipelineState state);

        /**
         * @brief 验证渲染通道是否有效
         * @return 是否所有通道都有效
         */
        bool validateRenderPasses() const;

        /**
         * @brief 初始化渲染通道
         * @param init_info 初始化信息
         * @return 是否初始化成功
         */
        virtual bool initializeRenderPasses(RenderPipelineInitInfo init_info) = 0;

        /**
         * @brief 清理渲染通道
         */
        virtual void cleanupRenderPasses();
    };
} // namespace Piccolo
