#include "runtime/function/render/passes/color_grading_pass.h"

// RHI types are available via included headers in render_pass.h; no direct include needed here

#include <color_grading_frag.h>
#include <post_process_vert.h>

// no stdexcept needed here

namespace Piccolo
{
    void ColorGradingPass::initialize(const RenderPassInitInfo* init_info)
    {
        RenderPass::initialize(nullptr);

        const ColorGradingPassInitInfo* color_grading_init_info = static_cast<const ColorGradingPassInitInfo*>(init_info);
        m_framebuffer.render_pass                             = color_grading_init_info->render_pass;

        setupDescriptorSetLayout();
        setupPipelines();
        setupDescriptorSet();
        updateAfterFramebufferRecreate(color_grading_init_info->input_attachment);
    }

    void ColorGradingPass::setupDescriptorSetLayout()
    {
        std::vector<RHIDescriptorSetLayoutBinding> bindings(2);
        RHIDescriptorSetLayoutBinding& input_attachment_binding = bindings[0];
        input_attachment_binding.binding                        = 0;
        input_attachment_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        input_attachment_binding.descriptorCount                = 1;
        input_attachment_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;

        RHIDescriptorSetLayoutBinding& lut_binding = bindings[1];
        lut_binding.binding                        = 1;
        lut_binding.descriptorType                 = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        lut_binding.descriptorCount                = 1;
        lut_binding.stageFlags                     = RHI_SHADER_STAGE_FRAGMENT_BIT;

        createDescriptorSetLayout(0, bindings);
    }

    void ColorGradingPass::setupPipelines()
    {
        std::vector<RHIDescriptorSetLayout*> set_layouts { m_descriptor_infos[0].layout };
        createPipelineLayout(0, set_layouts);

        RHIShader* vert_shader_module = m_rhi->createShaderModule(POST_PROCESS_VERT);
        RHIShader* frag_shader_module = m_rhi->createShaderModule(COLOR_GRADING_FRAG);

        createFullscreenTrianglePipeline(0,
                                         vert_shader_module,
                                         frag_shader_module,
                                         m_framebuffer.render_pass,
                                         _main_camera_subpass_color_grading);

        m_rhi->destroyShaderModule(vert_shader_module);
        m_rhi->destroyShaderModule(frag_shader_module);
    }

    void ColorGradingPass::setupDescriptorSet()
    {
        allocateDescriptorSet(0);
    }

    void ColorGradingPass::updateAfterFramebufferRecreate(RHIImageView* input_attachment)
    {
        RHIDescriptorImageInfo post_process_per_frame_input_attachment_info = {};
        post_process_per_frame_input_attachment_info.sampler                = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Nearest);
        post_process_per_frame_input_attachment_info.imageView              = input_attachment;
        post_process_per_frame_input_attachment_info.imageLayout            = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIDescriptorImageInfo color_grading_lut_image_info = {};
        color_grading_lut_image_info.sampler                = m_rhi->getOrCreateDefaultSampler(Default_Sampler_Linear);
        color_grading_lut_image_info.imageView              = m_global_render_resource->_color_grading_resource._color_grading_LUT_texture_image_view;
        color_grading_lut_image_info.imageLayout            = RHI_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        RHIWriteDescriptorSet post_process_descriptor_writes_info[2];

        RHIWriteDescriptorSet& post_process_descriptor_input_attachment_write_info = post_process_descriptor_writes_info[0];
        post_process_descriptor_input_attachment_write_info.sType                  = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        post_process_descriptor_input_attachment_write_info.pNext                  = nullptr;
        post_process_descriptor_input_attachment_write_info.dstSet                 = m_descriptor_infos[0].descriptor_set;
        post_process_descriptor_input_attachment_write_info.dstBinding             = 0;
        post_process_descriptor_input_attachment_write_info.dstArrayElement        = 0;
        post_process_descriptor_input_attachment_write_info.descriptorType         = RHI_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        post_process_descriptor_input_attachment_write_info.descriptorCount        = 1;
        post_process_descriptor_input_attachment_write_info.pImageInfo             = &post_process_per_frame_input_attachment_info;

        RHIWriteDescriptorSet& post_process_descriptor_lut_write_info = post_process_descriptor_writes_info[1];
        post_process_descriptor_lut_write_info.sType                  = RHI_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        post_process_descriptor_lut_write_info.pNext                  = nullptr;
        post_process_descriptor_lut_write_info.dstSet                 = m_descriptor_infos[0].descriptor_set;
        post_process_descriptor_lut_write_info.dstBinding             = 1;
        post_process_descriptor_lut_write_info.dstArrayElement        = 0;
        post_process_descriptor_lut_write_info.descriptorType         = RHI_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        post_process_descriptor_lut_write_info.descriptorCount        = 1;
        post_process_descriptor_lut_write_info.pImageInfo             = &color_grading_lut_image_info;

        m_rhi->updateDescriptorSets(
            sizeof(post_process_descriptor_writes_info) / sizeof(post_process_descriptor_writes_info[0]), post_process_descriptor_writes_info, 0, nullptr);
    }

    void ColorGradingPass::draw()
    {
        float color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
        m_rhi->pushEvent(m_rhi->getCurrentCommandBuffer(), "Color Grading", color);

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
