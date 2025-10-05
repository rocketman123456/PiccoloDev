#pragma once

#include <memory>
#include <string>
#include <vector>
#include <volk.h>

namespace Piccolo
{
    // 渲染命令构建器
    class GPURenderCommandBuilder
    {
    public:
        explicit GPURenderCommandBuilder(VkCommandBuffer command_buffer);
        ~GPURenderCommandBuilder() = default;

        // 渲染通道操作
        GPURenderCommandBuilder&
        beginRenderPass(VkRenderPass render_pass, VkFramebuffer framebuffer, const VkRect2D& render_area, const std::vector<VkClearValue>& clear_values);
        GPURenderCommandBuilder& endRenderPass();

        // 管道绑定
        GPURenderCommandBuilder& bindPipeline(VkPipeline pipeline, VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS);
        GPURenderCommandBuilder& bindDescriptorSets(
            VkPipelineLayout                    layout,
            uint32_t                            first_set,
            const std::vector<VkDescriptorSet>& descriptor_sets,
            const std::vector<uint32_t>&        dynamic_offsets = {}
        );

        // 视口和裁剪
        GPURenderCommandBuilder& setViewport(const VkViewport& viewport);
        GPURenderCommandBuilder& setViewports(const std::vector<VkViewport>& viewports);
        GPURenderCommandBuilder& setScissor(const VkRect2D& scissor);
        GPURenderCommandBuilder& setScissors(const std::vector<VkRect2D>& scissors);

        // 绘制命令
        GPURenderCommandBuilder& draw(uint32_t vertex_count, uint32_t instance_count = 1, uint32_t first_vertex = 0, uint32_t first_instance = 0);
        GPURenderCommandBuilder&
        drawIndexed(uint32_t index_count, uint32_t instance_count = 1, uint32_t first_index = 0, int32_t vertex_offset = 0, uint32_t first_instance = 0);
        GPURenderCommandBuilder& drawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride);
        GPURenderCommandBuilder& drawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride);

        // 顶点缓冲绑定
        GPURenderCommandBuilder& bindVertexBuffers(uint32_t first_binding, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets);
        GPURenderCommandBuilder& bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type);

        // 推送常量
        GPURenderCommandBuilder& pushConstants(VkPipelineLayout layout, VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const void* values);

        // 计算着色器调度
        GPURenderCommandBuilder& dispatch(uint32_t group_count_x, uint32_t group_count_y = 1, uint32_t group_count_z = 1);
        GPURenderCommandBuilder& dispatchIndirect(VkBuffer buffer, VkDeviceSize offset);

        // 内存屏障
        GPURenderCommandBuilder& pipelineBarrier(
            VkPipelineStageFlags                      src_stage_mask,
            VkPipelineStageFlags                      dst_stage_mask,
            VkDependencyFlags                         dependency_flags,
            const std::vector<VkMemoryBarrier>&       memory_barriers        = {},
            const std::vector<VkBufferMemoryBarrier>& buffer_memory_barriers = {},
            const std::vector<VkImageMemoryBarrier>&  image_memory_barriers  = {}
        );

        // 复制操作
        GPURenderCommandBuilder& copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, const std::vector<VkBufferCopy>& regions);
        GPURenderCommandBuilder&
        copyImage(VkImage src_image, VkImage dst_image, VkImageLayout src_layout, VkImageLayout dst_layout, const std::vector<VkImageCopy>& regions);
        GPURenderCommandBuilder&
        copyBufferToImage(VkBuffer src_buffer, VkImage dst_image, VkImageLayout dst_layout, const std::vector<VkBufferImageCopy>& regions);

        // 图像布局转换
        GPURenderCommandBuilder& transitionImageLayout(
            VkImage            image,
            VkImageLayout      old_layout,
            VkImageLayout      new_layout,
            VkImageAspectFlags aspect_mask      = VK_IMAGE_ASPECT_COLOR_BIT,
            uint32_t           base_mip_level   = 0,
            uint32_t           level_count      = 1,
            uint32_t           base_array_layer = 0,
            uint32_t           layer_count      = 1
        );

        // 获取命令缓冲区
        VkCommandBuffer getCommandBuffer() const { return m_command_buffer; }

    private:
        VkCommandBuffer m_command_buffer;
    };
} // namespace Piccolo
