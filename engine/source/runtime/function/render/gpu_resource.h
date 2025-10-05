#pragma once

#include <volk.h>

#include <cstdint>

namespace Piccolo
{
    static const uint32_t k_invalid_index = 0xffffffff;

    static const uint32_t k_buffers_pool_size                = 16384;
    static const uint32_t k_textures_pool_size               = 512;
    static const uint32_t k_render_passes_pool_size          = 256;
    static const uint32_t k_descriptor_set_layouts_pool_size = 128;
    static const uint32_t k_pipelines_pool_size              = 128;
    static const uint32_t k_shaders_pool_size                = 128;
    static const uint32_t k_descriptor_sets_pool_size        = 4096;
    static const uint32_t k_samplers_pool_size               = 32;

    static const uint8_t k_max_image_outputs          = 8;  // Maximum number of images/render_targets/fbo attachments usable.
    static const uint8_t k_max_descriptor_set_layouts = 8;  // Maximum number of layouts in the pipeline.
    static const uint8_t k_max_shader_stages          = 5;  // Maximum simultaneous shader stages. Applicable to all different type of pipelines.
    static const uint8_t k_max_descriptors_per_set    = 16; // Maximum list elements for both descriptor set layout and descriptor sets.
    static const uint8_t k_max_vertex_streams         = 16;
    static const uint8_t k_max_vertex_attributes      = 16;

    static const uint32_t k_submit_header_sentinel = 0xfefeb7ba;
    static const uint32_t k_max_resource_deletions = 64;

    using ResourceHandle = uint32_t;

    struct BufferHandle
    {
        ResourceHandle index;
    }; // struct BufferHandle

    struct TextureHandle
    {
        ResourceHandle index;
    }; // struct TextureHandle

    struct ShaderStateHandle
    {
        ResourceHandle index;
    }; // struct ShaderStateHandle

    struct SamplerHandle
    {
        ResourceHandle index;
    }; // struct SamplerHandle

    struct DescriptorSetLayoutHandle
    {
        ResourceHandle index;
    }; // struct DescriptorSetLayoutHandle

    struct DescriptorSetHandle
    {
        ResourceHandle index;
    }; // struct DescriptorSetHandle

    struct PipelineHandle
    {
        ResourceHandle index;
    }; // struct PipelineHandle

    struct RenderPassHandle
    {
        ResourceHandle index;
    }; // struct RenderPassHandle

    struct FramebufferHandle
    {
        ResourceHandle index;
    }; // struct PipelineHandle

    static BufferHandle              k_invalid_buffer {k_invalid_index};
    static TextureHandle             k_invalid_texture {k_invalid_index};
    static ShaderStateHandle         k_invalid_shader {k_invalid_index};
    static SamplerHandle             k_invalid_sampler {k_invalid_index};
    static DescriptorSetLayoutHandle k_invalid_layout {k_invalid_index};
    static DescriptorSetHandle       k_invalid_set {k_invalid_index};
    static PipelineHandle            k_invalid_pipeline {k_invalid_index};
    static RenderPassHandle          k_invalid_pass {k_invalid_index};
    static FramebufferHandle         k_invalid_framebuffer {k_invalid_index};

    // Resource creation structs ////////////////////////////////////////////////////

    //
    //
    struct Rect2D
    {
        float x      = 0.0f;
        float y      = 0.0f;
        float width  = 0.0f;
        float height = 0.0f;
    }; // struct Rect2D

    //
    //
    struct Rect2DInt
    {
        int16_t x      = 0;
        int16_t y      = 0;
        int16_t width  = 0;
        int16_t height = 0;
    }; // struct Rect2D

    //
    //
    struct Viewport
    {
        Rect2DInt rect;
        float     min_depth = 0.0f;
        float     max_depth = 0.0f;
    }; // struct Viewport

    //
    //
    struct ViewportState
    {
        uint32_t num_viewports = 0;
        uint32_t num_scissors  = 0;

        Viewport*  viewport = nullptr;
        Rect2DInt* scissors = nullptr;
    }; // struct ViewportState

    //
    //
    struct StencilOperationState
    {

        VkStencilOp fail         = VK_STENCIL_OP_KEEP;
        VkStencilOp pass         = VK_STENCIL_OP_KEEP;
        VkStencilOp depth_fail   = VK_STENCIL_OP_KEEP;
        VkCompareOp compare      = VK_COMPARE_OP_ALWAYS;
        uint32_t    compare_mask = 0xff;
        uint32_t    write_mask   = 0xff;
        uint32_t    reference    = 0xff;

    }; // struct StencilOperationState

    //
    //
    struct DepthStencilCreation
    {

        StencilOperationState front;
        StencilOperationState back;
        VkCompareOp           depth_comparison = VK_COMPARE_OP_ALWAYS;

        uint8_t depth_enable : 1;
        uint8_t depth_write_enable : 1;
        uint8_t stencil_enable : 1;
        uint8_t pad : 5;

        // Default constructor
        DepthStencilCreation()
            : depth_enable(0)
            , depth_write_enable(0)
            , stencil_enable(0)
        {}

        // DepthStencilCreation& set_depth(bool write, VkCompareOp comparison_test);

    }; // struct DepthStencilCreation

    struct BlendState
    {

        VkBlendFactor source_color      = VK_BLEND_FACTOR_ONE;
        VkBlendFactor destination_color = VK_BLEND_FACTOR_ONE;
        VkBlendOp     color_operation   = VK_BLEND_OP_ADD;

        VkBlendFactor source_alpha      = VK_BLEND_FACTOR_ONE;
        VkBlendFactor destination_alpha = VK_BLEND_FACTOR_ONE;
        VkBlendOp     alpha_operation   = VK_BLEND_OP_ADD;

        // ColorWriteEnabled::Mask color_write_mask = ColorWriteEnabled::All_mask;

        uint8_t blend_enabled : 1;
        uint8_t separate_blend : 1;
        uint8_t pad : 6;

        BlendState()
            : blend_enabled(0)
            , separate_blend(0)
        {}

        // BlendState& set_color(VkBlendFactor source_color, VkBlendFactor destination_color, VkBlendOp color_operation);
        // BlendState& set_alpha(VkBlendFactor source_color, VkBlendFactor destination_color, VkBlendOp color_operation);
        // BlendState& set_color_write_mask(ColorWriteEnabled::Mask value);

    }; // struct BlendState

    struct BlendStateCreation
    {

        BlendState blend_states[k_max_image_outputs];
        uint32_t   active_states = 0;

        // BlendStateCreation& reset();
        // BlendState&         add_blend_state();

    }; // BlendStateCreation

    //
    //
    struct RasterizationCreation
    {
        VkCullModeFlagBits cull_mode = VK_CULL_MODE_NONE;
        VkFrontFace        front     = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        // FillMode::Enum     fill      = FillMode::Solid;
    }; // struct RasterizationCreation

    //
    //
    struct BufferCreation
    {

        VkBufferUsageFlags type_flags = 0;
        // ResourceUsageType::Enum usage        = ResourceUsageType::Immutable;
        uint32_t size         = 0;
        uint32_t persistent   = 0;
        uint32_t device_only  = 0;
        void*    initial_data = nullptr;

        const char* name = nullptr;

        // BufferCreation& reset();
        // BufferCreation& set(VkBufferUsageFlags flags, ResourceUsageType::Enum usage, u32 size);
        // BufferCreation& set_data(void* data);
        // BufferCreation& set_name(const char* name);
        // BufferCreation& set_persistent(bool value);
        // BufferCreation& set_device_only(bool value);

    }; // struct BufferCreation

    //
    //
    struct TextureCreation
    {

        void*    initial_data = nullptr;
        uint16_t width        = 1;
        uint16_t height       = 1;
        uint16_t depth        = 1;
        uint8_t  mipmaps      = 1;
        uint8_t  flags        = 0; // TextureFlags bitmasks

        VkFormat format = VK_FORMAT_UNDEFINED;
        // TextureType::Enum type   = TextureType::Texture2D;

        TextureHandle alias = k_invalid_texture;

        const char* name = nullptr;

        // TextureCreation& set_size(u16 width, u16 height, u16 depth);
        // TextureCreation& set_flags(u8 mipmaps, u8 flags);
        // TextureCreation& set_format_type(VkFormat format, TextureType::Enum type);
        // TextureCreation& set_name(const char* name);
        // TextureCreation& set_data(void* data);
        // TextureCreation& set_alias(TextureHandle alias);
    }; // struct TextureCreation

    //
    //
    struct SamplerCreation
    {
        VkFilter            min_filter = VK_FILTER_NEAREST;
        VkFilter            mag_filter = VK_FILTER_NEAREST;
        VkSamplerMipmapMode mip_filter = VK_SAMPLER_MIPMAP_MODE_NEAREST;

        VkSamplerAddressMode address_mode_u = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode address_mode_v = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode address_mode_w = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        const char* name = nullptr;

        // SamplerCreation& set_min_mag_mip(VkFilter min, VkFilter mag, VkSamplerMipmapMode mip);
        // SamplerCreation& set_address_mode_u(VkSamplerAddressMode u);
        // SamplerCreation& set_address_mode_uv(VkSamplerAddressMode u, VkSamplerAddressMode v);
        // SamplerCreation& set_address_mode_uvw(VkSamplerAddressMode u, VkSamplerAddressMode v, VkSamplerAddressMode w);
        // SamplerCreation& set_name(const char* name);
    }; // struct SamplerCreation

    //
    //
    struct ShaderStage
    {
        const char*           code      = nullptr;
        uint32_t              code_size = 0;
        VkShaderStageFlagBits type      = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;

    }; // struct ShaderStage

    struct ShaderStateCreation
    {

        ShaderStage stages[k_max_shader_stages];

        const char* name = nullptr;

        uint32_t stages_count = 0;
        uint32_t spv_input    = 0;

        // Building helpers
        // ShaderStateCreation& reset();
        // ShaderStateCreation& set_name(const char* name);
        // ShaderStateCreation& add_stage(const char* code, sizet code_size, VkShaderStageFlagBits type);
        // ShaderStateCreation& set_spv_input(bool value);

    }; // struct ShaderStateCreation

    //
    //
    struct DescriptorSetLayoutCreation
    {
        //
        // A single descriptor binding. It can be relative to one or more resources of the same type.
        //
        struct Binding
        {

            VkDescriptorType type  = VK_DESCRIPTOR_TYPE_MAX_ENUM;
            uint16_t         index = 0;
            uint16_t         count = 0;
            const char*      name  = nullptr; // Comes from external memory.
        }; // struct Binding

        Binding  bindings[k_max_descriptors_per_set];
        uint32_t num_bindings = 0;
        uint32_t set_index    = 0;
        bool     bindless     = false;
        bool     dynamic      = false;

        const char* name = nullptr;

        // Building helpers
        // DescriptorSetLayoutCreation& reset();
        // DescriptorSetLayoutCreation& add_binding(const Binding& binding);
        // DescriptorSetLayoutCreation& add_binding(VkDescriptorType type, u32 index, u32 count, cstring name);
        // DescriptorSetLayoutCreation& add_binding_at_index(const Binding& binding, int index);
        // DescriptorSetLayoutCreation& set_name(cstring name);
        // DescriptorSetLayoutCreation& set_set_index(u32 index);
    }; // struct DescriptorSetLayoutCreation

    //
    //
    struct DescriptorSetCreation
    {

        ResourceHandle resources[k_max_descriptors_per_set];
        SamplerHandle  samplers[k_max_descriptors_per_set];
        uint16_t       bindings[k_max_descriptors_per_set];

        DescriptorSetLayoutHandle layout;
        uint32_t                  num_resources = 0;

        const char* name = nullptr;

        // Building helpers
        // DescriptorSetCreation& reset();
        // DescriptorSetCreation& set_layout(DescriptorSetLayoutHandle layout);
        // DescriptorSetCreation& texture(TextureHandle texture, u16 binding);
        // DescriptorSetCreation& buffer(BufferHandle buffer, u16 binding);
        // DescriptorSetCreation& texture_sampler(TextureHandle texture, SamplerHandle sampler, u16 binding); // TODO: separate samplers from textures
        // DescriptorSetCreation& set_name(cstring name);
    }; // struct DescriptorSetCreation

    //
    //
    struct DescriptorSetUpdate
    {
        DescriptorSetHandle descriptor_set;

        uint32_t frame_issued = 0;
    }; // DescriptorSetUpdate

    //
    //
    struct VertexAttribute
    {
        uint16_t location = 0;
        uint16_t binding  = 0;
        uint32_t offset   = 0;
        // VertexComponentFormat::Enum format   = VertexComponentFormat::Count;
    }; // struct VertexAttribute

    //
    //
    struct VertexStream
    {

        uint16_t binding = 0;
        uint16_t stride  = 0;
        // VertexInputRate::Enum input_rate = VertexInputRate::Count;

    }; // struct VertexStream

    //
    //
    struct VertexInputCreation
    {

        uint32_t num_vertex_streams    = 0;
        uint32_t num_vertex_attributes = 0;

        VertexStream    vertex_streams[k_max_vertex_streams];
        VertexAttribute vertex_attributes[k_max_vertex_attributes];

        // VertexInputCreation& reset();
        // VertexInputCreation& add_vertex_stream(const VertexStream& stream);
        // VertexInputCreation& add_vertex_attribute(const VertexAttribute& attribute);
    }; // struct VertexInputCreation

    //
    //
    struct RenderPassOutput
    {
        VkFormat      color_formats[k_max_image_outputs];
        VkImageLayout color_final_layouts[k_max_image_outputs];
        // RenderPassOperation::Enum color_operations[k_max_image_outputs];

        VkFormat      depth_stencil_format;
        VkImageLayout depth_stencil_final_layout;

        uint32_t num_color_formats;

        // RenderPassOperation::Enum depth_operation   = RenderPassOperation::DontCare;
        // RenderPassOperation::Enum stencil_operation = RenderPassOperation::DontCare;

        // RenderPassOutput& reset();
        // RenderPassOutput& color(VkFormat format, VkImageLayout layout, RenderPassOperation::Enum load_op);
        // RenderPassOutput& depth(VkFormat format, VkImageLayout layout);
        // RenderPassOutput& set_depth_stencil_operations(RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil);

    }; // struct RenderPassOutput

    //
    //
    struct RenderPassCreation
    {
        uint16_t num_render_targets = 0;

        VkFormat      color_formats[k_max_image_outputs];
        VkImageLayout color_final_layouts[k_max_image_outputs];
        // RenderPassOperation::Enum color_operations[k_max_image_outputs];

        VkFormat      depth_stencil_format = VK_FORMAT_UNDEFINED;
        VkImageLayout depth_stencil_final_layout;

        // RenderPassOperation::Enum depth_operation   = RenderPassOperation::DontCare;
        // RenderPassOperation::Enum stencil_operation = RenderPassOperation::DontCare;

        const char* name = nullptr;

        // RenderPassCreation& reset();
        // RenderPassCreation& add_attachment(VkFormat format, VkImageLayout layout, RenderPassOperation::Enum load_op);
        // RenderPassCreation& set_depth_stencil_texture(VkFormat format, VkImageLayout layout);
        // RenderPassCreation& set_name(const char* name);
        // RenderPassCreation& set_depth_stencil_operations(RenderPassOperation::Enum depth, RenderPassOperation::Enum stencil);
    }; // struct RenderPassCreation

    //
    //
    struct FramebufferCreation
    {
        RenderPassHandle render_pass;

        uint16_t num_render_targets = 0;

        TextureHandle output_textures[k_max_image_outputs];
        TextureHandle depth_stencil_texture = {k_invalid_index};

        uint16_t width  = 0;
        uint16_t height = 0;

        float   scale_x = 1.f;
        float   scale_y = 1.f;
        uint8_t resize  = 1;

        const char* name = nullptr;

        // FramebufferCreation& reset();
        // FramebufferCreation& add_render_texture(TextureHandle texture);
        // FramebufferCreation& set_depth_stencil_texture(TextureHandle texture);
        // FramebufferCreation& set_scaling(f32 scale_x, f32 scale_y, u8 resize);
        // FramebufferCreation& set_name(const char* name);
    }; // struct RenderPassCreation

    //
    //
    struct PipelineCreation
    {
        RasterizationCreation rasterization;
        DepthStencilCreation  depth_stencil;
        BlendStateCreation    blend_state;
        VertexInputCreation   vertex_input;
        ShaderStateCreation   shaders;

        RenderPassOutput          render_pass;
        DescriptorSetLayoutHandle descriptor_set_layout[k_max_descriptor_set_layouts];
        const ViewportState*      viewport = nullptr;

        uint32_t num_active_layouts = 0;

        const char* name = nullptr;

        // PipelineCreation& add_descriptor_set_layout(DescriptorSetLayoutHandle handle);
        // RenderPassOutput& render_pass_output();
    }; // struct PipelineCreation

} // namespace Piccolo
