#include "runtime/function/render/passes/vignette_pass.h"

// RHI types are available via included headers in render_pass.h; no direct include needed here

#include <vignette_frag.h>
#include <vignette_vert.h>

// no stdexcept needed here

namespace Piccolo
{
    void VignettePass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const VignettePassInitInfo* vignette_init_info = static_cast<const VignettePassInitInfo*>(init_info);
        m_framebuffer.render_pass                        = vignette_init_info->render_pass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        updateAfterFramebufferRecreate(vignette_init_info->input_attachment);
    }

    void VignettePass::setupDescriptorSetLayout()
    {
        std::vector<RHIDescriptorSetLayoutBinding> bindings(1);
        RHIDescriptorSetLayoutBinding& input_attachment_binding = bindings[0];
        input_attachment_binding.binding                        = 0;
        input_attachment_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        input_attachment_binding.descriptorCount                = 1;
        input_attachment_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;
        createDescriptorSetLayout(0, bindings);
    }

    void VignettePass::setupPipelines()
    {
        std::vector<RHIDescriptorSetLayout*> set_layouts { m_descriptor_infos[0].layout };
        createPipelineLayout(0, set_layouts);

        RHIShader* vert_shader_module = m_rhi->createShaderModule(VIGNETTE_VERT);
        RHIShader* frag_shader_module = m_rhi->createShaderModule(VIGNETTE_FRAG);

        createFullscreenTrianglePipeline(0,
                                         vert_shader_module,
                                         frag_shader_module,
                                         m_framebuffer.render_pass,
                                         _main_camera_subpass_vignette);

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);
    }

    void VignettePass::setupDescriptorSet()
    {
        allocateDescriptorSet(0);
    }

    void VignettePass::updateAfterFramebufferRecreate(RHIImageView* input_attachment)
    {
        RHIDescriptorImageInfo post_process_per_frame_input_attachment_info = {};
        post_process_per_frame_input_attachment_info.sampler =
            m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
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
        post_process_descriptor_input_attachment_write_info.descriptorType  = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        post_process_descriptor_input_attachment_write_info.descriptorCount = 1;
        post_process_descriptor_input_attachment_write_info.pImageInfo = &post_process_per_frame_input_attachment_info;

        m_rhi->updateDescriptorSets(sizeof(post_process_descriptor_writes_info) /
                                    sizeof(post_process_descriptor_writes_info[0]),
                                    post_process_descriptor_writes_info,
                                    0,
                                    nullptr);
    }

    void VignettePass::draw()
    {
        float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "Vignette", color);

        m_rhi->cmdBindPipelinePFN(m_rhi->getCurrentCommandBuffer(), RHI_PIPELINE_BIND_POINT_GRAPHICS, m_render_pipelines[0].pipeline);
        m_rhi->cmdSetViewportPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().viewport);
        m_rhi->cmdSetScissorPFN(m_rhi->getCurrentCommandBuffer(), 0, 1, m_rhi->getSwapchainInfo().scissor);
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
