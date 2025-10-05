#include "runtime/function/render/utils/gpu_render_command_builder.h"

#include "runtime/core/base/macro.h"

#include <algorithm>
#include <stdexcept>

namespace Piccolo
{
    // GPURenderCommandBuilder 实现
    GPURenderCommandBuilder::GPURenderCommandBuilder(VkCommandBuffer command_buffer)
        : m_command_buffer(command_buffer)
    {}

    GPURenderCommandBuilder& GPURenderCommandBuilder::beginRenderPass(
        VkRenderPass                     render_pass,
        VkFramebuffer                    framebuffer,
        const VkRect2D&                  render_area,
        const std::vector<VkClearValue>& clear_values
    )
    {
        VkRenderPassBeginInfo render_pass_info {};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass      = render_pass;
        render_pass_info.framebuffer     = framebuffer;
        render_pass_info.renderArea      = render_area;
        render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
        render_pass_info.pClearValues    = clear_values.data();

        vkCmdBeginRenderPass(m_command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::endRenderPass()
    {
        vkCmdEndRenderPass(m_command_buffer);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::bindPipeline(VkPipeline pipeline, VkPipelineBindPoint bind_point)
    {
        vkCmdBindPipeline(m_command_buffer, bind_point, pipeline);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::bindDescriptorSets(
        VkPipelineLayout                    layout,
        uint32_t                            first_set,
        const std::vector<VkDescriptorSet>& descriptor_sets,
        const std::vector<uint32_t>&        dynamic_offsets
    )
    {
        vkCmdBindDescriptorSets(
            m_command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            layout,
            first_set,
            static_cast<uint32_t>(descriptor_sets.size()),
            descriptor_sets.data(),
            static_cast<uint32_t>(dynamic_offsets.size()),
            dynamic_offsets.data()
        );
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::setViewport(const VkViewport& viewport)
    {
        vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::setViewports(const std::vector<VkViewport>& viewports)
    {
        vkCmdSetViewport(m_command_buffer, 0, static_cast<uint32_t>(viewports.size()), viewports.data());
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::setScissor(const VkRect2D& scissor)
    {
        vkCmdSetScissor(m_command_buffer, 0, 1, &scissor);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::setScissors(const std::vector<VkRect2D>& scissors)
    {
        vkCmdSetScissor(m_command_buffer, 0, static_cast<uint32_t>(scissors.size()), scissors.data());
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
    {
        vkCmdDraw(m_command_buffer, vertex_count, instance_count, first_vertex, first_instance);
        return *this;
    }

    GPURenderCommandBuilder&
    GPURenderCommandBuilder::drawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
    {
        vkCmdDrawIndexed(m_command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::drawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride)
    {
        vkCmdDrawIndirect(m_command_buffer, buffer, offset, draw_count, stride);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::drawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t draw_count, uint32_t stride)
    {
        vkCmdDrawIndexedIndirect(m_command_buffer, buffer, offset, draw_count, stride);
        return *this;
    }

    GPURenderCommandBuilder&
    GPURenderCommandBuilder::bindVertexBuffers(uint32_t first_binding, const std::vector<VkBuffer>& buffers, const std::vector<VkDeviceSize>& offsets)
    {
        vkCmdBindVertexBuffers(m_command_buffer, first_binding, static_cast<uint32_t>(buffers.size()), buffers.data(), offsets.data());
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::bindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType index_type)
    {
        vkCmdBindIndexBuffer(m_command_buffer, buffer, offset, index_type);
        return *this;
    }

    GPURenderCommandBuilder&
    GPURenderCommandBuilder::pushConstants(VkPipelineLayout layout, VkShaderStageFlags stage_flags, uint32_t offset, uint32_t size, const void* values)
    {
        vkCmdPushConstants(m_command_buffer, layout, stage_flags, offset, size, values);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
    {
        vkCmdDispatch(m_command_buffer, group_count_x, group_count_y, group_count_z);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::dispatchIndirect(VkBuffer buffer, VkDeviceSize offset)
    {
        vkCmdDispatchIndirect(m_command_buffer, buffer, offset);
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::pipelineBarrier(
        VkPipelineStageFlags                      src_stage_mask,
        VkPipelineStageFlags                      dst_stage_mask,
        VkDependencyFlags                         dependency_flags,
        const std::vector<VkMemoryBarrier>&       memory_barriers,
        const std::vector<VkBufferMemoryBarrier>& buffer_memory_barriers,
        const std::vector<VkImageMemoryBarrier>&  image_memory_barriers
    )
    {
        vkCmdPipelineBarrier(
            m_command_buffer,
            src_stage_mask,
            dst_stage_mask,
            dependency_flags,
            static_cast<uint32_t>(memory_barriers.size()),
            memory_barriers.data(),
            static_cast<uint32_t>(buffer_memory_barriers.size()),
            buffer_memory_barriers.data(),
            static_cast<uint32_t>(image_memory_barriers.size()),
            image_memory_barriers.data()
        );
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::copyBuffer(VkBuffer src_buffer, VkBuffer dst_buffer, const std::vector<VkBufferCopy>& regions)
    {
        vkCmdCopyBuffer(m_command_buffer, src_buffer, dst_buffer, static_cast<uint32_t>(regions.size()), regions.data());
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::copyImage(
        VkImage                         src_image,
        VkImage                         dst_image,
        VkImageLayout                   src_layout,
        VkImageLayout                   dst_layout,
        const std::vector<VkImageCopy>& regions
    )
    {
        vkCmdCopyImage(m_command_buffer, src_image, src_layout, dst_image, dst_layout, static_cast<uint32_t>(regions.size()), regions.data());
        return *this;
    }

    GPURenderCommandBuilder&
    GPURenderCommandBuilder::copyBufferToImage(VkBuffer src_buffer, VkImage dst_image, VkImageLayout dst_layout, const std::vector<VkBufferImageCopy>& regions)
    {
        vkCmdCopyBufferToImage(m_command_buffer, src_buffer, dst_image, dst_layout, static_cast<uint32_t>(regions.size()), regions.data());
        return *this;
    }

    GPURenderCommandBuilder& GPURenderCommandBuilder::transitionImageLayout(
        VkImage            image,
        VkImageLayout      old_layout,
        VkImageLayout      new_layout,
        VkImageAspectFlags aspect_mask,
        uint32_t           base_mip_level,
        uint32_t           level_count,
        uint32_t           base_array_layer,
        uint32_t           layer_count
    )
    {
        VkImageMemoryBarrier barrier {};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = old_layout;
        barrier.newLayout                       = new_layout;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = image;
        barrier.subresourceRange.aspectMask     = aspect_mask;
        barrier.subresourceRange.baseMipLevel   = base_mip_level;
        barrier.subresourceRange.levelCount     = level_count;
        barrier.subresourceRange.baseArrayLayer = base_array_layer;
        barrier.subresourceRange.layerCount     = layer_count;

        vkCmdPipelineBarrier(m_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        return *this;
    }
} // namespace Piccolo
