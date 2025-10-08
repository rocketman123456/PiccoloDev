#pragma once

#include "runtime/function/render/gpu_shader.h"

#include <memory>
#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    // 着色器阶段配置
    struct GPUShaderStageConfig
    {
        ShaderType               type;
        std::string              shader_path;
        std::string              entry_point = "main";
        std::vector<std::string> defines;
    };

    // 管道配置结构 (保持向后兼容)
    struct GPUPipelineConfig
    {
        std::string name;
        std::string description;

        std::vector<GPUShaderStageConfig> shader_stages;
    };

    // 顶点输入配置
    struct VertexInputConfig
    {
        std::vector<VkVertexInputBindingDescription>   bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
    };

    // 输入装配配置
    struct InputAssemblyConfig
    {
        VkPrimitiveTopology topology                 = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        bool                primitive_restart_enable = false;
    };

    // 光栅化配置
    struct RasterizationConfig
    {
        bool            depth_clamp_enable         = false;
        bool            rasterizer_discard_enable  = false;
        VkPolygonMode   polygon_mode               = VK_POLYGON_MODE_FILL;
        float           line_width                 = 1.0f;
        VkCullModeFlags cull_mode                  = VK_CULL_MODE_BACK_BIT;
        VkFrontFace     front_face                 = VK_FRONT_FACE_CLOCKWISE;
        bool            depth_bias_enable          = false;
        float           depth_bias_constant_factor = 0.0f;
        float           depth_bias_clamp           = 0.0f;
        float           depth_bias_slope_factor    = 0.0f;
    };

    // 多重采样配置
    struct MultisampleConfig
    {
        VkSampleCountFlagBits rasterization_samples    = VK_SAMPLE_COUNT_1_BIT;
        bool                  sample_shading_enable    = false;
        float                 min_sample_shading       = 1.0f;
        VkSampleMask*         sample_mask              = nullptr;
        bool                  alpha_to_coverage_enable = false;
        bool                  alpha_to_one_enable      = false;
    };

    // 深度模板配置
    struct DepthStencilConfig
    {
        bool             depth_test_enable        = true;
        bool             depth_write_enable       = true;
        VkCompareOp      depth_compare_op         = VK_COMPARE_OP_LESS;
        bool             depth_bounds_test_enable = false;
        float            min_depth_bounds         = 0.0f;
        float            max_depth_bounds         = 1.0f;
        bool             stencil_test_enable      = false;
        VkStencilOpState front                    = {};
        VkStencilOpState back                     = {};
    };

    // 颜色混合配置
    struct ColorBlendConfig
    {
        bool                                             logic_op_enable = false;
        VkLogicOp                                        logic_op        = VK_LOGIC_OP_COPY;
        std::vector<VkPipelineColorBlendAttachmentState> attachments;
        float                                            blend_constants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    };

    // 动态状态配置
    struct DynamicStateConfig
    {
        std::vector<VkDynamicState> states = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    };

    // 完整的管道配置
    struct GPUPipelineBuilderConfig
    {
        std::string name;
        std::string description;

        std::vector<GPUShaderStageConfig> shader_stages;

        VertexInputConfig   vertex_input;
        InputAssemblyConfig input_assembly;
        RasterizationConfig rasterization;
        MultisampleConfig   multisample;
        DepthStencilConfig  depth_stencil;
        ColorBlendConfig    color_blend;
        DynamicStateConfig  dynamic_state;

        // 描述符集布局
        std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
        std::vector<VkPushConstantRange>   push_constant_ranges;

        // 渲染通道
        VkRenderPass render_pass = VK_NULL_HANDLE;
        uint32_t     subpass     = 0;
    };

    // 管道构建器类
    class GPUPipelineBuilder
    {
    public:
        explicit GPUPipelineBuilder(VkDevice device);
        ~GPUPipelineBuilder() = default;

        // 设置基本配置
        GPUPipelineBuilder& setName(const std::string& name);
        GPUPipelineBuilder& setDescription(const std::string& description);

        // 着色器配置
        GPUPipelineBuilder& addShaderStage(const GPUShaderStageConfig& stage);
        GPUPipelineBuilder& addVertexShader(const std::string& path, const std::string& entry_point = "main");
        GPUPipelineBuilder& addFragmentShader(const std::string& path, const std::string& entry_point = "main");
        GPUPipelineBuilder& addComputeShader(const std::string& path, const std::string& entry_point = "main");

        // 顶点输入配置
        GPUPipelineBuilder& setVertexInput(const VertexInputConfig& config);
        GPUPipelineBuilder& addVertexBinding(uint32_t binding, uint32_t stride, VkVertexInputRate input_rate = VK_VERTEX_INPUT_RATE_VERTEX);
        GPUPipelineBuilder& addVertexAttribute(uint32_t location, uint32_t binding, VkFormat format, uint32_t offset);

        // 输入装配配置
        GPUPipelineBuilder& setInputAssembly(const InputAssemblyConfig& config);
        GPUPipelineBuilder& setTopology(VkPrimitiveTopology topology);

        // 光栅化配置
        GPUPipelineBuilder& setRasterization(const RasterizationConfig& config);
        GPUPipelineBuilder& setCullMode(VkCullModeFlags cull_mode);
        GPUPipelineBuilder& setPolygonMode(VkPolygonMode polygon_mode);

        // 多重采样配置
        GPUPipelineBuilder& setMultisample(const MultisampleConfig& config);
        GPUPipelineBuilder& setSampleCount(VkSampleCountFlagBits samples);

        // 深度模板配置
        GPUPipelineBuilder& setDepthStencil(const DepthStencilConfig& config);
        GPUPipelineBuilder& enableDepthTest(bool enable = true);
        GPUPipelineBuilder& enableDepthWrite(bool enable = true);

        // 颜色混合配置
        GPUPipelineBuilder& setColorBlend(const ColorBlendConfig& config);
        GPUPipelineBuilder& addColorBlendAttachment(const VkPipelineColorBlendAttachmentState& attachment);
        GPUPipelineBuilder& addDefaultColorBlendAttachment();

        // 动态状态配置
        GPUPipelineBuilder& setDynamicState(const DynamicStateConfig& config);
        GPUPipelineBuilder& addDynamicState(VkDynamicState state);

        // 描述符和推送常量
        GPUPipelineBuilder& addDescriptorSetLayout(VkDescriptorSetLayout layout);
        GPUPipelineBuilder& addPushConstantRange(const VkPushConstantRange& range);

        // 渲染通道
        GPUPipelineBuilder& setRenderPass(VkRenderPass render_pass, uint32_t subpass = 0);

        // 构建管道
        VkPipeline       buildGraphicsPipeline();
        VkPipeline       buildComputePipeline();
        VkPipelineLayout getPipelineLayout() const { return m_pipeline_layout; }

        // 重置构建器
        void reset();

    private:
        void createShaders();
        void createPipelineLayout();
        void cleanupShaders();

        VkDevice                 m_device;
        GPUPipelineBuilderConfig m_config;

        std::vector<std::shared_ptr<GPUShader>> m_shaders;
        VkPipelineLayout                        m_pipeline_layout = VK_NULL_HANDLE;
    };

    // 预定义配置工厂
    class GPUPipelineConfigFactory
    {
    public:
        // 基础三角形渲染管道
        static GPUPipelineBuilderConfig createBasicTrianglePipeline();

        static GPUPipelineBuilderConfig createAdvancedTrianglePipeline();

        // 带深度测试的管道
        static GPUPipelineBuilderConfig createDepthTestPipeline();

        // 线框渲染管道
        static GPUPipelineBuilderConfig createWireframePipeline();

        // 计算管道
        static GPUPipelineBuilderConfig createComputePipeline(const std::string& compute_shader_path);

        // 后处理管道
        static GPUPipelineBuilderConfig createPostProcessPipeline();
    };
} // namespace Piccolo
