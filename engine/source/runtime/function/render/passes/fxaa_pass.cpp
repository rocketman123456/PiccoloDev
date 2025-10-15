#include "runtime/function/render/passes/fxaa_pass.h"
#include "runtime/core/base/macro.h"

#include <fxaa_frag.h>
#include <fxaa_vert.h>

// no stdexcept needed here

namespace Piccolo
{
    void FXAAPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const FXAAPassInitInfo* fxaa_init_info = static_cast<const FXAAPassInitInfo*>(init_info);
        if (init_info == nullptr)
        {
            LOG_ERROR("fxaa _init_info is nullptr");
            return;
        }

        m_framebuffer.render_pass = fxaa_init_info->render_pass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        updateAfterFramebufferRecreate(fxaa_init_info->input_attachment);
    }

    void FXAAPass::setupDescriptorSetLayout()
    {
        std::vector<RHIDescriptorSetLayoutBinding> bindings(1);
        RHIDescriptorSetLayoutBinding& in_color_binding = bindings[0];
        in_color_binding.binding                        = 0;
        in_color_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        in_color_binding.descriptorCount                = 1;
        in_color_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;
        createDescriptorSetLayout(0, bindings);
    }

    void FXAAPass::setupPipelines()
    {
        std::vector<RHIDescriptorSetLayout*> set_layouts { m_descriptor_infos[0].layout };
        createPipelineLayout(0, set_layouts);

        RHIShader* vert_shader_module = m_rhi->createShaderModule(FXAA_VERT);
        RHIShader* frag_shader_module = m_rhi->createShaderModule(FXAA_FRAG);

        createFullscreenTrianglePipeline(0,
                                         vert_shader_module,
                                         frag_shader_module,
                                         m_framebuffer.render_pass,
                                         _main_camera_subpass_fxaa);

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);
    }

    void FXAAPass::setupDescriptorSet()
    {
        allocateDescriptorSet(0);
    }

    void FXAAPass::updateAfterFramebufferRecreate(RHIImageView* input_attachment)
    {
        RHIDescriptorImageInfo post_process_per_frame_input_attachment_info = {};
        post_process_per_frame_input_attachment_info.sampler = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Linear);
        post_process_per_frame_input_attachment_info.imageView   = input_attachment;
        post_process_per_frame_input_attachment_info.imageLayout = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIWriteDescriptorSet post_process_descriptor_writes_info[1];

        RHIWriteDescriptorSet& post_process_descriptor_input_attachment_write_info =
            post_process_descriptor_writes_info[0];
        post_process_descriptor_input_attachment_write_info.sType           = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        post_process_descriptor_input_attachment_write_info.pNext           = nullptr;
        post_process_descriptor_input_attachment_write_info.dstSet          = m_descriptor_infos[0].descriptor_set;
        post_process_descriptor_input_attachment_write_info.dstBinding      = 0;
        post_process_descriptor_input_attachment_write_info.dstArrayElement = 0;
        post_process_descriptor_input_attachment_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        post_process_descriptor_input_attachment_write_info.descriptorCount = 1;
        post_process_descriptor_input_attachment_write_info.pImageInfo = &post_process_per_frame_input_attachment_info;

        m_rhi->updateDescriptorSets(sizeof(post_process_descriptor_writes_info) /
                                    sizeof(post_process_descriptor_writes_info[0]),
                                    post_process_descriptor_writes_info,
                                    0,
                                    nullptr);
    }

    void FXAAPass::draw()
    {
        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "FXAA", color);

        RHIViewport viewport = {0.0,
                               0.0,
                               static_cast<float>(m_rhi->getSwapchainInfo().extent.width),
                               static_cast<float>(m_rhi->getSwapchainInfo().extent.height),
                               0.0,
                               1.0};
        int32_t    x        = static_cast<int32_t>(m_rhi->getSwapchainInfo().viewport->x);
        int32_t    y        = static_cast<int32_t>(m_rhi->getSwapchainInfo().viewport->y);
        uint32_t   width    = static_cast<uint32_t>(m_rhi->getSwapchainInfo().viewport->width);
        uint32_t   height   = static_cast<uint32_t>(m_rhi->getSwapchainInfo().viewport->height);
        RHIRect2D   scissor  = {x, y, width, height};

        m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, m_render_pipelines[0].pipeline);
        m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, &viewport);
        m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, &scissor);
        m_rhi->cmdBindDescriptorSetsPFN(m_rhi->getCurrentCommandBuffer(),
                                        RHI_PIPELINE_BIND_POINT_GRAPHICS,
                                        m_render_pipelines[0].layout,
                                        0,
                                        1,
                                        &m_descriptor_infos[0].descriptor_set,
                                        0,
                                        nullptr);

        m_rhi->cmdDraw(m_rhi->getCurrentCommandBuffer(), 3, 1, 0, 0);

        m_rhi->popEvent(m_rhi->getCurrentCommandBuffer());
    }

} // namespace Piccolo
