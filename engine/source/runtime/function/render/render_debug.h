#pragma once

#include "runtime/function/render/interface/rhi.h"
#include "runtime/core/base/macro.h"

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cfloat>
#include <functional>

namespace Piccolo
{
    /**
     * @brief 渲染调试器
     * 提供渲染系统的调试和性能分析功能
     */
    class RenderDebugger
    {
    public:
        /**
         * @brief 构造函数
         */
        RenderDebugger() = default;

        /**
         * @brief 析构函数
         */
        ~RenderDebugger() = default;

        // ========== 调试组管理 ==========

        /**
         * @brief 开始调试组
         * @param name 调试组名称
         */
        void beginDebugGroup(const std::string& name);

        /**
         * @brief 结束调试组
         */
        void endDebugGroup();

        /**
         * @brief 插入调试标记
         * @param name 标记名称
         */
        void insertDebugMarker(const std::string& name);

        // ========== 帧管理 ==========

        /**
         * @brief 开始新帧
         */
        void beginFrame();

        /**
         * @brief 结束当前帧
         */
        void endFrame();

        /**
         * @brief 获取当前帧号
         * @return 帧号
         */
        uint32_t getCurrentFrame() const { return m_current_frame; }

        // ========== 统计信息 ==========

        /**
         * @brief 获取帧统计信息
         * @return 统计信息字符串
         */
        std::string getFrameStatistics() const;

        /**
         * @brief 获取GPU内存使用情况
         * @return 内存使用信息字符串
         */
        std::string getMemoryStatistics() const;

        /**
         * @brief 重置统计信息
         */
        void resetStatistics();

    private:
        uint32_t m_current_frame = 0;                    ///< 当前帧号
        std::vector<std::string> m_debug_groups;         ///< 调试组栈
        std::chrono::high_resolution_clock::time_point m_frame_start_time; ///< 帧开始时间

        /**
         * @brief 获取当前时间戳
         * @return 时间戳字符串
         */
        std::string getCurrentTimestamp() const;
    };

    /**
     * @brief 渲染性能分析器
     * 提供详细的性能分析和计时功能
     */
    class RenderProfiler
    {
    public:
        /**
         * @brief 构造函数
         * @param rhi 渲染硬件接口
         */
        explicit RenderProfiler(std::shared_ptr<RHI> rhi);

        /**
         * @brief 析构函数
         */
        ~RenderProfiler() = default;

        // ========== CPU计时器 ==========

        /**
         * @brief 开始CPU计时
         * @param name 计时器名称
         */
        void beginCPUTimer(const std::string& name);

        /**
         * @brief 结束CPU计时
         * @param name 计时器名称
         */
        void endCPUTimer(const std::string& name);

        /**
         * @brief 获取CPU计时结果
         * @param name 计时器名称
         * @return 平均时间（毫秒）
         */
        float getCPUTimerResult(const std::string& name) const;

        // ========== GPU计时器 ==========

        /**
         * @brief 开始GPU计时
         * @param name 计时器名称
         */
        void beginGPUTimer(const std::string& name);

        /**
         * @brief 结束GPU计时
         * @param name 计时器名称
         */
        void endGPUTimer(const std::string& name);

        /**
         * @brief 获取GPU计时结果
         * @param name 计时器名称
         * @return 平均时间（毫秒）
         */
        float getGPUTimerResult(const std::string& name) const;

        // ========== 统计报告 ==========

        /**
         * @brief 生成性能报告
         * @return 性能报告字符串
         */
        std::string generateReport() const;

        /**
         * @brief 重置所有计时器
         */
        void reset();

        /**
         * @brief 更新GPU查询结果
         */
        void updateGPUQueries();

    private:
        std::shared_ptr<RHI> m_rhi; ///< 渲染硬件接口

        // CPU计时器数据
        struct CPUTimerData
        {
            std::chrono::high_resolution_clock::time_point start_time;
            std::vector<float> samples;
            float average_time = 0.0f;
            float min_time = FLT_MAX;
            float max_time = 0.0f;
        };

        // GPU计时器数据
        struct GPUTimerData
        {
            RHIBuffer* query_buffer = nullptr;
            uint32_t start_query = 0;
            uint32_t end_query = 0;
            std::vector<float> samples;
            float average_time = 0.0f;
            float min_time = FLT_MAX;
            float max_time = 0.0f;
            bool is_active = false;
        };

        std::unordered_map<std::string, CPUTimerData> m_cpu_timers; ///< CPU计时器
        std::unordered_map<std::string, GPUTimerData> m_gpu_timers; ///< GPU计时器

        /**
         * @brief 计算统计信息
         * @param samples 样本数据
         * @param average 平均值
         * @param min_val 最小值
         * @param max_val 最大值
         */
        void calculateStatistics(const std::vector<float>& samples, float& average, float& min_val, float& max_val) const;
    };

    /**
     * @brief 渲染错误处理器
     * 提供统一的错误处理和报告功能
     */
    class RenderErrorHandler
    {
    public:
        /**
         * @brief 错误类型枚举
         */
        enum class ErrorType : uint8_t
        {
            SHADER_COMPILATION,    ///< 着色器编译错误
            BUFFER_CREATION,       ///< 缓冲区创建错误
            TEXTURE_LOADING,       ///< 纹理加载错误
            PIPELINE_CREATION,     ///< 管线创建错误
            RENDER_PASS_ERROR,     ///< 渲染通道错误
            MEMORY_ALLOCATION,     ///< 内存分配错误
            API_ERROR,             ///< API调用错误
            UNKNOWN                ///< 未知错误
        };

        /**
         * @brief 错误回调函数类型
         */
        using ErrorCallback = std::function<void(ErrorType, const std::string&, const std::string&)>;

        /**
         * @brief 构造函数
         */
        RenderErrorHandler() = default;

        /**
         * @brief 析构函数
         */
        ~RenderErrorHandler() = default;

        // ========== 错误处理接口 ==========

        /**
         * @brief 处理错误
         * @param type 错误类型
         * @param message 错误消息
         * @param details 错误详情（可选）
         */
        void handleError(ErrorType type, const std::string& message, const std::string& details = "");

        /**
         * @brief 设置错误回调函数
         * @param callback 回调函数
         */
        void setErrorCallback(ErrorCallback callback) { m_error_callback = callback; }

        /**
         * @brief 获取错误统计信息
         * @return 错误统计字符串
         */
        std::string getErrorStatistics() const;

        /**
         * @brief 清除错误统计
         */
        void clearErrorStatistics();

        /**
         * @brief 检查是否有错误
         * @return 是否有错误
         */
        bool hasErrors() const { return !m_error_counts.empty(); }

    private:
        ErrorCallback m_error_callback; ///< 错误回调函数
        std::unordered_map<ErrorType, uint32_t> m_error_counts; ///< 错误计数

        /**
         * @brief 获取错误类型名称
         * @param type 错误类型
         * @return 错误类型名称
         */
        std::string getErrorTypeName(ErrorType type) const;
    };

    /**
     * @brief 渲染调试管理器
     * 统一管理所有调试相关功能
     */
    class RenderDebugManager
    {
    public:
        /**
         * @brief 构造函数
         * @param rhi 渲染硬件接口
         */
        explicit RenderDebugManager(std::shared_ptr<RHI> rhi);

        /**
         * @brief 析构函数
         */
        ~RenderDebugManager() = default;

        // ========== 组件访问接口 ==========

        /**
         * @brief 获取调试器
         * @return 调试器的引用
         */
        RenderDebugger& getDebugger() { return m_debugger; }

        /**
         * @brief 获取性能分析器
         * @return 性能分析器的引用
         */
        RenderProfiler& getProfiler() { return m_profiler; }

        /**
         * @brief 获取错误处理器
         * @return 错误处理器的引用
         */
        RenderErrorHandler& getErrorHandler() { return m_error_handler; }

        // ========== 统一管理接口 ==========

        /**
         * @brief 初始化调试管理器
         * @return 是否初始化成功
         */
        bool initialize();

        /**
         * @brief 更新调试管理器
         * @param delta_time 帧间隔时间
         */
        void update(float delta_time);

        /**
         * @brief 清理调试管理器
         */
        void cleanup();

        /**
         * @brief 生成完整的调试报告
         * @return 调试报告字符串
         */
        std::string generateDebugReport() const;

        /**
         * @brief 重置所有调试数据
         */
        void reset();

    private:
        std::shared_ptr<RHI> m_rhi; ///< 渲染硬件接口

        RenderDebugger m_debugger;        ///< 调试器
        RenderProfiler m_profiler;        ///< 性能分析器
        RenderErrorHandler m_error_handler; ///< 错误处理器

        bool m_initialized = false; ///< 是否已初始化
    };
} // namespace Piccolo
