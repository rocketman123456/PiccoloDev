#pragma once

#include "runtime/core/base/macro.h"
#include "runtime/function/render/render_type.h"

#include <string>
#include <unordered_map>

namespace Piccolo
{
    /**
     * @brief 渲染质量等级
     */
    enum class RenderQuality : uint8_t
    {
        LOW = 0,     ///< 低质量
        MEDIUM,      ///< 中等质量
        HIGH,        ///< 高质量
        ULTRA,       ///< 超高质量
        CUSTOM       ///< 自定义
    };

    /**
     * @brief 渲染配置结构体
     * 包含所有渲染相关的配置选项
     */
    struct RenderConfig
    {
        // ========== 渲染管线配置 ==========
        struct PipelineConfig
        {
            RENDER_PIPELINE_TYPE pipeline_type = RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE; ///< 渲染管线类型
            bool enable_fxaa = false;           ///< 是否启用FXAA抗锯齿
            bool enable_ssao = false;           ///< 是否启用SSAO环境光遮蔽
            bool enable_ssr = false;            ///< 是否启用屏幕空间反射
            bool enable_temporal_aa = false;    ///< 是否启用时间抗锯齿
        } pipeline;

        // ========== 光照配置 ==========
        struct LightingConfig
        {
            uint32_t max_directional_lights = 4;    ///< 最大方向光数量
            uint32_t max_point_lights = 64;         ///< 最大点光源数量
            uint32_t max_spot_lights = 16;          ///< 最大聚光灯数量
            uint32_t shadow_map_size = 2048;        ///< 阴影贴图大小
            bool enable_cascaded_shadows = true;    ///< 是否启用级联阴影
            uint32_t shadow_cascade_count = 4;      ///< 阴影级联数量
            float shadow_bias = 0.001f;             ///< 阴影偏移
        } lighting;

        // ========== 后处理配置 ==========
        struct PostProcessConfig
        {
            bool enable_tone_mapping = true;        ///< 是否启用色调映射
            bool enable_color_grading = true;       ///< 是否启用颜色分级
            bool enable_vignette = false;           ///< 是否启用暗角效果
            bool enable_bloom = false;              ///< 是否启用泛光效果
            float exposure = 1.0f;                  ///< 曝光值
            float gamma = 2.2f;                     ///< 伽马值
        } post_process;

        // ========== 性能配置 ==========
        struct PerformanceConfig
        {
            uint32_t max_meshes = 4096;             ///< 最大网格数量
            uint32_t max_materials = 1024;          ///< 最大材质数量
            uint32_t max_textures = 2048;           ///< 最大纹理数量
            uint32_t max_lights = 256;              ///< 最大光源数量
            bool enable_instancing = true;          ///< 是否启用实例化渲染
            bool enable_frustum_culling = true;     ///< 是否启用视锥剔除
            bool enable_occlusion_culling = false;  ///< 是否启用遮挡剔除
        } performance;

        // ========== 调试配置 ==========
        struct DebugConfig
        {
            bool enable_debug_draw = false;         ///< 是否启用调试绘制
            bool enable_wireframe = false;          ///< 是否启用线框模式
            bool enable_statistics = false;         ///< 是否显示统计信息
            bool enable_gpu_timing = false;         ///< 是否启用GPU时间统计
            bool enable_memory_stats = false;       ///< 是否显示内存统计
        } debug;

        // ========== 显示配置 ==========
        struct DisplayConfig
        {
            uint32_t window_width = 1920;           ///< 窗口宽度
            uint32_t window_height = 1080;          ///< 窗口高度
            bool fullscreen = false;                ///< 是否全屏
            bool vsync = true;                      ///< 是否启用垂直同步
            uint32_t target_fps = 60;               ///< 目标帧率
        } display;

        /**
         * @brief 获取渲染质量等级
         * @return 渲染质量等级
         */
        RenderQuality getQualityLevel() const;

        /**
         * @brief 设置渲染质量等级
         * @param quality 质量等级
         */
        void setQualityLevel(RenderQuality quality);

        /**
         * @brief 验证配置的有效性
         * @return 配置是否有效
         */
        bool isValid() const;

        /**
         * @brief 重置为默认配置
         */
        void resetToDefaults();
    };

    /**
     * @brief 渲染配置管理器
     * 负责加载、保存和管理渲染配置
     */
    class RenderConfigManager
    {
    public:
        /**
         * @brief 构造函数
         */
        RenderConfigManager() = default;

        /**
         * @brief 析构函数
         */
        ~RenderConfigManager() = default;

        // ========== 配置管理接口 ==========

        /**
         * @brief 从文件加载配置
         * @param file_path 配置文件路径
         * @return 是否加载成功
         */
        bool loadConfig(const std::string& file_path);

        /**
         * @brief 保存配置到文件
         * @param file_path 配置文件路径
         * @return 是否保存成功
         */
        bool saveConfig(const std::string& file_path) const;

        /**
         * @brief 获取当前配置
         * @return 当前配置的引用
         */
        const RenderConfig& getConfig() const { return m_config; }

        /**
         * @brief 设置配置
         * @param config 新的配置
         */
        void setConfig(const RenderConfig& config) { m_config = config; }

        // ========== 配置访问接口 ==========

        /**
         * @brief 获取渲染管线配置
         * @return 管线配置的引用
         */
        const RenderConfig::PipelineConfig& getPipelineConfig() const { return m_config.pipeline; }

        /**
         * @brief 获取光照配置
         * @return 光照配置的引用
         */
        const RenderConfig::LightingConfig& getLightingConfig() const { return m_config.lighting; }

        /**
         * @brief 获取后处理配置
         * @return 后处理配置的引用
         */
        const RenderConfig::PostProcessConfig& getPostProcessConfig() const { return m_config.post_process; }

        /**
         * @brief 获取性能配置
         * @return 性能配置的引用
         */
        const RenderConfig::PerformanceConfig& getPerformanceConfig() const { return m_config.performance; }

        /**
         * @brief 获取调试配置
         * @return 调试配置的引用
         */
        const RenderConfig::DebugConfig& getDebugConfig() const { return m_config.debug; }

        /**
         * @brief 获取显示配置
         * @return 显示配置的引用
         */
        const RenderConfig::DisplayConfig& getDisplayConfig() const { return m_config.display; }

        // ========== 配置修改接口 ==========

        /**
         * @brief 设置渲染管线类型
         * @param pipeline_type 管线类型
         */
        void setPipelineType(RENDER_PIPELINE_TYPE pipeline_type) { m_config.pipeline.pipeline_type = pipeline_type; }

        /**
         * @brief 启用/禁用FXAA
         * @param enable 是否启用
         */
        void setFXAAEnabled(bool enable) { m_config.pipeline.enable_fxaa = enable; }

        /**
         * @brief 设置阴影贴图大小
         * @param size 贴图大小
         */
        void setShadowMapSize(uint32_t size) { m_config.lighting.shadow_map_size = size; }

        /**
         * @brief 设置窗口大小
         * @param width 宽度
         * @param height 高度
         */
        void setWindowSize(uint32_t width, uint32_t height) {
            m_config.display.window_width = width;
            m_config.display.window_height = height;
        }

        /**
         * @brief 启用/禁用垂直同步
         * @param enable 是否启用
         */
        void setVSyncEnabled(bool enable) { m_config.display.vsync = enable; }

        // ========== 预设配置接口 ==========

        /**
         * @brief 应用低质量预设
         */
        void applyLowQualityPreset();

        /**
         * @brief 应用中等质量预设
         */
        void applyMediumQualityPreset();

        /**
         * @brief 应用高质量预设
         */
        void applyHighQualityPreset();

        /**
         * @brief 应用超高质量预设
         */
        void applyUltraQualityPreset();

        /**
         * @brief 应用性能优化预设
         */
        void applyPerformancePreset();

        /**
         * @brief 应用质量优先预设
         */
        void applyQualityPreset();

    private:
        RenderConfig m_config; ///< 当前配置

        /**
         * @brief 从JSON对象加载配置
         * @param json_obj JSON对象
         * @return 是否加载成功
         */
        bool loadFromJson(const std::string& json_str);

        /**
         * @brief 将配置保存为JSON字符串
         * @return JSON字符串
         */
        std::string saveToJson() const;

        /**
         * @brief 验证配置值的范围
         * @return 配置是否有效
         */
        bool validateConfig() const;
    };
} // namespace Piccolo
