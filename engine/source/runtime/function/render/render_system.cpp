#include "runtime/function/render/render_system.h"

#include "runtime/function/render/gpu_command_pool.h"
#include "runtime/function/render/gpu_context.h"
#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/gpu_pipeline.h"
#include "runtime/function/render/gpu_pipeline_manager.h"
#include "runtime/function/render/gpu_render_pass.h"
#include "runtime/function/render/gpu_render_resource_manager.h"
#include "runtime/function/render/gpu_render_state_manager.h"
#include "runtime/function/render/gpu_swap_chain.h"
#include "runtime/function/render/gpu_sync_object.h"
#include "runtime/function/render/utils/gpu_buffer_utils.h"
#include "runtime/function/render/utils/gpu_pipeline_builder.h"
#include "runtime/function/render/utils/gpu_render_pass_builder.h"
#include "runtime/function/render/window_system.h"

#include "runtime/function/input/input_manager.h"
#include "runtime/function/render/camera.h"

// ImGui
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

#include "runtime/function/profiler/cpu_profiler.h"
#include "runtime/function/profiler/gpu_profiler.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/gpu_render_resource.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include <array>
#include <vector>

namespace Piccolo
{
    // ========== 静态数据 ==========

    /**
     * @brief 默认三角形顶点数据
     *
     * 用于演示渲染系统的基础三角形，包含位置和颜色信息
     * 位置使用2D坐标，颜色使用RGB格式
     */
    // const std::vector<Vertex> vertices = {
    //     {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}}, // 底部中心点，白色
    //     { {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}}, // 右上角，绿色
    //     {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}  // 左上角，蓝色
    // };

    // Ground plane on XZ at y=0 with per-vertex normals for Phong shading
    const std::vector<Vertex> vertices = {
        {{-50.0f, 0.0f, -50.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.6f, 0.3f}},
        {{ 50.0f, 0.0f, -50.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.7f, 0.3f}},
        {{ 50.0f, 0.0f,  50.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.8f, 0.3f}},
        {{-50.0f, 0.0f,  50.0f}, {0.0f, 1.0f, 0.0f}, {0.3f, 0.7f, 0.3f}},
    };

    const std::vector<uint16_t> indices = {0, 1, 2, 2, 3, 0};

    void RenderSystem::initialize()
    {
        LOG_INFO("开始初始化渲染系统...");

        // ========== 第一阶段：初始化核心GPU组件 ==========
        LOG_INFO("初始化GPU上下文和设备...");
        m_context    = std::make_shared<GPUContext>();
        m_device     = std::make_shared<GPUDevice>(m_context->getInstance());
        m_swap_chain = std::make_shared<GPUSwapChain>(m_device->getPhysicalDevice(), m_device->getDevice(), m_device->getSurface());

        // ========== 第二阶段：初始化管理工具类 ==========
        LOG_INFO("初始化渲染管理工具...");
        m_resource_manager = std::make_shared<GPURenderResourceManager>(m_device->getDevice(), m_device->getPhysicalDevice());
        m_state_manager    = std::make_shared<GPURenderStateManager>(m_device->getDevice());

        // ========== 第三阶段：初始化命令和同步对象 ==========
        // 使用交换链图像数量作为最大并发帧数，确保每个图像都有独立的命令缓冲区和同步对象
        uint32_t max_frames_in_flight = static_cast<uint32_t>(m_swap_chain->getImages().size());
        LOG_INFO("初始化命令池和同步对象，最大并发帧数: {}", max_frames_in_flight);

        m_command_pool = std::make_shared<GPUCommandPool>(m_device->getDevice(), max_frames_in_flight);
        m_sync_object  = std::make_shared<GPUSyncObject>(m_device->getDevice(), max_frames_in_flight);

        // ========== 第四阶段：初始化渲染资源 ==========
        LOG_INFO("初始化渲染资源...");
        initializeRenderResources();

        // ========== 初始化相机与输入 ==========
        m_camera        = std::make_unique<RenderCamera>();
        m_input_manager = std::make_unique<InputManager>();
        m_input_manager->initialize(g_runtime_global_context.m_window_system.get(), m_camera.get());

        // ========== 第五阶段：初始化 ImGui ==========
        LOG_INFO("准备初始化 ImGui...");
        initializeImGui();
        LOG_INFO("ImGui 初始化步骤调用完成");

        LOG_INFO("渲染系统初始化完成");
    }

    void RenderSystem::clear()
    {
        LOG_INFO("开始清理渲染系统...");

        // 等待所有GPU操作完成，确保安全清理
        if (m_device && m_device->getDevice() != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device->getDevice());
        }

        // 先清理 ImGui
        destroyImGui();

        // ========== 按相反顺序清理资源 ==========

        // 清理渲染资源
        if (m_vertex_buffer.isValid())
        {
            LOG_INFO("清理顶点缓冲区...");
            m_vertex_buffer.destroy(m_device->getDevice());
        }

        if (m_index_buffer.isValid())
        {
            LOG_INFO("清理索引缓冲区...");
            m_index_buffer.destroy(m_device->getDevice());
        }

        // 清理相机 UBO 与描述符资源
        if (m_camera_ubo.isValid())
        {
            LOG_INFO("清理相机 UBO 缓冲区...");
            m_camera_ubo.destroy(m_device->getDevice());
        }
        if (m_camera_descriptor_pool != VK_NULL_HANDLE)
        {
            LOG_INFO("清理相机描述符池...");
            vkDestroyDescriptorPool(m_device->getDevice(), m_camera_descriptor_pool, nullptr);
            m_camera_descriptor_pool = VK_NULL_HANDLE;
            m_camera_descriptor_set  = VK_NULL_HANDLE;
        }

        // 清理管理工具类
        LOG_INFO("清理渲染管理工具...");
        m_state_manager.reset();
        m_resource_manager.reset();

        // 说明：相机描述符集布局由资源管理器负责销毁，这里不重复销毁
        m_camera_set_layout = VK_NULL_HANDLE;

        // 清理同步和命令对象
        LOG_INFO("清理同步对象和命令池...");
        m_sync_object.reset();
        m_command_pool.reset();

        // 清理渲染管线
        LOG_INFO("清理渲染管线...");
        m_pipeline.reset();
        m_render_pass.reset();

        // 清理交换链
        LOG_INFO("清理交换链...");
        m_swap_chain.reset();

        // 清理设备和上下文
        LOG_INFO("清理GPU设备和上下文...");
        m_device.reset();
        m_context.reset();

        LOG_INFO("渲染系统清理完成");
    }

    void RenderSystem::tick(float /*dt*/)
    {
        // 开始新帧的性能分析
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;
        cpu_profiler->beginFrame(m_current_frame);

        // 获取当前帧的同步对象和命令缓冲区
        auto in_flight_fence           = m_sync_object->getInFlightFence(m_current_frame);
        auto image_available_semaphore = m_sync_object->getImageAvailableSemaphore(m_current_frame);
        auto command_buffer            = m_command_pool->getCommandBuffer(m_current_frame);

        // ========== 第一阶段：获取交换链图像 ==========
        uint32_t image_index;
        VkResult result = acquireNextImage(in_flight_fence, image_available_semaphore, image_index);
        if (result != VK_SUCCESS)
        {
            return; // 获取图像失败或需要重建交换链
        }

        // 更新相机与输入
        updateCameraAndInput(0.0f);

        // ========== 第二阶段：记录渲染命令 ==========
        if (!recordRenderCommands(command_buffer, image_index))
        {
            LOG_ERROR("记录渲染命令失败");
            return;
        }

        // ========== 第三阶段：提交命令缓冲区 ==========
        if (!submitCommandBuffer(command_buffer, image_available_semaphore, in_flight_fence))
        {
            LOG_ERROR("提交命令缓冲区失败");
            return;
        }

        // ========== 第四阶段：呈现图像 ==========
        result = presentImage(image_index);
        if (result != VK_SUCCESS)
        {
            return; // 呈现失败或需要重建交换链
        }

        // 结束帧性能分析并更新帧索引
        cpu_profiler->endFrame(m_current_frame);
        m_current_frame = (m_current_frame + 1) % m_sync_object->getMaxFramesInFlight();
    }

    void RenderSystem::initializeRenderResources()
    {
        // 创建默认渲染通道
        createDefaultRenderPass();

        // 创建渲染资源（包括相机 UBO 和顶点/索引缓冲）
        createRenderResource();

        // 创建默认管道
        createDefaultPipeline();
    }

    void RenderSystem::createDefaultRenderPass()
    {
        // 使用新的工厂方法创建基础颜色渲染通道
        auto color_format = m_swap_chain->getImageFormat();

        auto config   = GPURenderPassConfigFactory::createBasicColorPass(color_format);
        m_render_pass = std::make_shared<GPURenderPass>(m_device->getDevice(), config);
    }

    void RenderSystem::createDefaultPipeline()
    {
        auto config = GPUPipelineConfigFactory::createAdvancedTrianglePipeline();
        if (m_camera_set_layout != VK_NULL_HANDLE)
        {
            config.descriptor_set_layouts.push_back(m_camera_set_layout);
        }
        m_pipeline = std::make_shared<GPUPipeline>(m_device->getDevice(), config, m_render_pass->getRenderPass());
    }

    void RenderSystem::createRenderResource()
    {
        // 使用新的GPUBuffer类创建顶点缓冲区
        size_t buffer_size = sizeof(vertices[0]) * vertices.size();
        size_t index_size  = sizeof(indices[0]) * indices.size();

        // 初始化顶点缓冲区（使用设备本地内存以获得更好的性能）
        if (!m_vertex_buffer.initialize(
                m_device->getDevice(),
                m_device->getPhysicalDevice(),
                buffer_size,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            ))
        {
            LOG_ERROR("Failed to initialize vertex buffer");
            return;
        }

        if (!m_index_buffer.initialize(
                m_device->getDevice(),
                m_device->getPhysicalDevice(),
                index_size,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            ))
        {
            LOG_ERROR("Failed to initialize index buffer");
            return;
        }

        // 上传顶点数据
        if (!m_vertex_buffer.uploadData(
                m_device->getDevice(),
                m_device->getPhysicalDevice(),
                m_command_pool->getCommandPool(),
                m_device->getGraphicsQueue(),
                const_cast<void*>(static_cast<const void*>(vertices.data())),
                buffer_size
            ))
        {
            LOG_ERROR("Failed to upload vertex data");
            return;
        }

        if (!m_index_buffer.uploadData(
                m_device->getDevice(),
                m_device->getPhysicalDevice(),
                m_command_pool->getCommandPool(),
                m_device->getGraphicsQueue(),
                const_cast<void*>(static_cast<const void*>(indices.data())),
                index_size
            ))
        {
            LOG_ERROR("Failed to upload index data");
            return;
        }

        LOG_INFO("顶点缓冲区创建和上传完成，大小: {} 字节", buffer_size);

        // ====== 创建相机 UBO、描述符布局/池/集合 ======
        // 1) 描述符布局 set=0, binding=0 uniform buffer, vertex stage
        {
            VkDescriptorSetLayoutBinding binding {};
            binding.binding            = 0;
            binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount    = 1;
            binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
            binding.pImmutableSamplers = nullptr;

            std::vector<VkDescriptorSetLayoutBinding> bindings {binding};
            m_camera_set_layout = m_resource_manager->createDescriptorSetLayout("CameraSetLayout", bindings);
        }

        // 2) 创建相机 UBO（主机可见，直接更新）
        {
            const size_t camera_size = sizeof(glm::mat4) * 2;
            if (!m_camera_ubo.initialize(
                    m_device->getDevice(),
                    m_device->getPhysicalDevice(),
                    camera_size,
                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                ))
            {
                LOG_ERROR("Failed to create camera UBO");
                return;
            }
        }

        // 3) 创建描述符池与集合
        {
            VkDescriptorPoolSize pool_size {};
            pool_size.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            pool_size.descriptorCount = 1;

            VkDescriptorPoolCreateInfo pool_info {};
            pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.maxSets       = 1;
            pool_info.poolSizeCount = 1;
            pool_info.pPoolSizes    = &pool_size;

            if (vkCreateDescriptorPool(m_device->getDevice(), &pool_info, nullptr, &m_camera_descriptor_pool) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to create camera descriptor pool");
                return;
            }

            VkDescriptorSetAllocateInfo alloc_info {};
            alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            alloc_info.descriptorPool     = m_camera_descriptor_pool;
            alloc_info.descriptorSetCount = 1;
            alloc_info.pSetLayouts        = &m_camera_set_layout;

            if (vkAllocateDescriptorSets(m_device->getDevice(), &alloc_info, &m_camera_descriptor_set) != VK_SUCCESS)
            {
                LOG_ERROR("Failed to allocate camera descriptor set");
                return;
            }

            VkDescriptorBufferInfo buffer_info {};
            buffer_info.buffer = m_camera_ubo.getBuffer();
            buffer_info.offset = 0;
            buffer_info.range  = VK_WHOLE_SIZE;

            VkWriteDescriptorSet write {};
            write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet          = m_camera_descriptor_set;
            write.dstBinding      = 0;
            write.dstArrayElement = 0;
            write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo     = &buffer_info;

            vkUpdateDescriptorSets(m_device->getDevice(), 1, &write, 0, nullptr);
        }
    }

    // ========== 渲染循环辅助方法实现 ==========

    VkResult RenderSystem::acquireNextImage(VkFence in_flight_fence, VkSemaphore image_available_semaphore, uint32_t& image_index)
    {
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;

        cpu_profiler->beginTimestamp("Frame Wait");

        // 等待上一帧完成
        vkWaitForFences(m_device->getDevice(), 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device->getDevice(), 1, &in_flight_fence);

        // 获取交换链图像
        VkResult result =
            vkAcquireNextImageKHR(m_device->getDevice(), m_swap_chain->getSwapchain(), UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);

        cpu_profiler->endTimestamp("Frame Wait");

        // 处理交换链过期或次优情况
        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            LOG_INFO("交换链过期，需要重建");
            recreateSwapChain();
            return VK_ERROR_OUT_OF_DATE_KHR;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            LOG_ERROR("获取交换链图像失败: {}", static_cast<int>(result));
            return result;
        }

        return VK_SUCCESS;
    }

    bool RenderSystem::recordRenderCommands(VkCommandBuffer command_buffer, uint32_t image_index)
    {
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;

        // 重置命令缓冲区
        cpu_profiler->beginTimestamp("Command Buffer Reset");
        vkResetCommandBuffer(command_buffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
        cpu_profiler->endTimestamp("Command Buffer Reset");

        // 开始记录命令缓冲区
        VkCommandBufferBeginInfo begin_info {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        cpu_profiler->beginTimestamp("Command Buffer Begin");
        if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        {
            LOG_ERROR("开始记录命令缓冲区失败");
            return false;
        }
        cpu_profiler->endTimestamp("Command Buffer Begin");

        // 记录渲染命令
        cpu_profiler->beginTimestamp("Command Record");
        // 开始 ImGui 帧（生成 DrawData）
        beginImGuiFrame();

        // 先记录场景渲染（内部将调用 ImGui 绘制）
        m_command_pool->recordRenderCommands(command_buffer, image_index);
        cpu_profiler->endTimestamp("Command Record");

        // 结束记录命令缓冲区
        cpu_profiler->beginTimestamp("Command Buffer End");
        if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        {
            LOG_ERROR("结束记录命令缓冲区失败");
            return false;
        }
        cpu_profiler->endTimestamp("Command Buffer End");

        return true;
    }

    // ========================= ImGui 集成 =========================
    void RenderSystem::initializeImGui()
    {
        if (m_imgui_initialized)
            return;

        LOG_INFO("ImGui: Create context");
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // 启用停靠
        ImGui::StyleColorsDark();

        // 创建 descriptor pool 给 ImGui 使用
        VkDescriptorPoolSize pool_sizes[] = {
            {               VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {         VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {         VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {  VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {  VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {      VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
        };

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets       = 1000 * (uint32_t)(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
        pool_info.poolSizeCount = (uint32_t)(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
        pool_info.pPoolSizes    = pool_sizes;
        LOG_INFO("ImGui: Create descriptor pool");
        if (vkCreateDescriptorPool(m_device->getDevice(), &pool_info, nullptr, &m_imgui_descriptor_pool) != VK_SUCCESS)
        {
            LOG_ERROR("ImGui descriptor pool 创建失败");
            return;
        }

        // 初始化平台/渲染器后端
        auto window = g_runtime_global_context.m_window_system->getWindow();
        LOG_INFO("ImGui: Init GLFW backend");
        if (!ImGui_ImplGlfw_InitForVulkan(window, true))
        {
            LOG_ERROR("ImGui_ImplGlfw_InitForVulkan 失败");
            return;
        }

        // 如果使用动态加载（如 volk），需要提供函数加载器
        LOG_INFO("ImGui: Load Vulkan functions");
        ImGui_ImplVulkan_LoadFunctions(
            [](const char* function_name, void* instance) -> PFN_vkVoidFunction { return vkGetInstanceProcAddr((VkInstance)instance, function_name); },
            (void*)m_context->getInstance()
        );

        ImGui_ImplVulkan_InitInfo init_info {};
        init_info.Instance        = m_context->getInstance();
        init_info.PhysicalDevice  = m_device->getPhysicalDevice();
        init_info.Device          = m_device->getDevice();
        init_info.QueueFamily     = m_device->getGraphicsQueueFamily();
        init_info.Queue           = m_device->getGraphicsQueue();
        init_info.PipelineCache   = VK_NULL_HANDLE;
        init_info.DescriptorPool  = m_imgui_descriptor_pool;
        init_info.Subpass         = 0;
        init_info.MinImageCount   = (uint32_t)m_swap_chain->getImages().size();
        init_info.ImageCount      = (uint32_t)m_swap_chain->getImages().size();
        init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator       = nullptr;
        init_info.CheckVkResultFn = nullptr;

        // 这里直接使用主渲染通道，后续可改为独立 UI pass
        LOG_INFO("ImGui: Init Vulkan backend");
        if (!ImGui_ImplVulkan_Init(&init_info, m_render_pass->getRenderPass()))
        {
            LOG_ERROR("ImGui_ImplVulkan_Init 失败");
            return;
        }

        // 上传字体
        {
            LOG_INFO("ImGui: Upload fonts");
            VkCommandBufferAllocateInfo alloc_info {};
            alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            alloc_info.commandPool        = m_command_pool->getCommandPool();
            alloc_info.commandBufferCount = 1;

            VkCommandBuffer cmd;
            vkAllocateCommandBuffers(m_device->getDevice(), &alloc_info, &cmd);

            VkCommandBufferBeginInfo begin_info {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            vkBeginCommandBuffer(cmd, &begin_info);
            ImGui_ImplVulkan_CreateFontsTexture(cmd);
            vkEndCommandBuffer(cmd);

            VkSubmitInfo submit_info {};
            submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submit_info.commandBufferCount = 1;
            submit_info.pCommandBuffers    = &cmd;
            vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submit_info, VK_NULL_HANDLE);
            vkDeviceWaitIdle(m_device->getDevice());
            ImGui_ImplVulkan_DestroyFontUploadObjects();

            vkFreeCommandBuffers(m_device->getDevice(), m_command_pool->getCommandPool(), 1, &cmd);
        }

        m_imgui_initialized = true;
        LOG_INFO("ImGui 初始化完成");
    }

    void RenderSystem::destroyImGui()
    {
        if (!m_imgui_initialized)
            return;

        vkDeviceWaitIdle(m_device->getDevice());
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        if (m_imgui_descriptor_pool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(m_device->getDevice(), m_imgui_descriptor_pool, nullptr);
            m_imgui_descriptor_pool = VK_NULL_HANDLE;
        }
        m_imgui_initialized = false;
    }

    void RenderSystem::beginImGuiFrame()
    {
        if (!m_imgui_initialized)
            return;
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 创建主视口 DockSpace（贯穿中央节点以透出场景）
        ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        // 示例：简单显示 FPS
        ImGui::Begin("Stats");
        ImGui::Text("Frame %u", m_current_frame);
        ImGui::End();

        ImGui::Render();
    }

    void RenderSystem::recordImGuiDrawData(VkCommandBuffer command_buffer)
    {
        if (!m_imgui_initialized)
            return;

        // 将 ImGui 绘制记录到当前命令缓冲（要求处于一个 render pass 内）
        ImDrawData* draw_data = ImGui::GetDrawData();
        if (draw_data == nullptr)
            return;
        ImGui_ImplVulkan_RenderDrawData(draw_data, command_buffer);
    }

    bool RenderSystem::submitCommandBuffer(VkCommandBuffer command_buffer, VkSemaphore image_available_semaphore, VkFence in_flight_fence)
    {
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;

        VkSubmitInfo submit_info {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // 设置等待信号量
        VkSemaphore          wait_semaphores[] = {image_available_semaphore};
        VkPipelineStageFlags wait_stages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount         = 1;
        submit_info.pWaitSemaphores            = wait_semaphores;
        submit_info.pWaitDstStageMask          = wait_stages;

        // 设置命令缓冲区
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;

        // 设置信号信号量
        VkSemaphore signal_semaphores[]  = {m_sync_object->getRenderFinishedSemaphore(m_current_frame)};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        // 提交命令缓冲区
        cpu_profiler->beginTimestamp("Queue Submit");
        VkResult result = vkQueueSubmit(m_device->getGraphicsQueue(), 1, &submit_info, in_flight_fence);
        cpu_profiler->endTimestamp("Queue Submit");

        if (result != VK_SUCCESS)
        {
            LOG_ERROR("提交命令缓冲区失败: {}", static_cast<int>(result));
            return false;
        }

        return true;
    }

    VkResult RenderSystem::presentImage(uint32_t image_index)
    {
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;

        VkPresentInfoKHR present_info {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        // 设置等待信号量
        VkSemaphore signal_semaphores[] = {m_sync_object->getRenderFinishedSemaphore(m_current_frame)};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        // 设置交换链
        VkSwapchainKHR swap_chains[] = {m_swap_chain->getSwapchain()};
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;
        present_info.pImageIndices   = &image_index;

        // 呈现图像
        cpu_profiler->beginTimestamp("Present");
        VkResult result = vkQueuePresentKHR(m_device->getPresentQueue(), &present_info);
        cpu_profiler->endTimestamp("Present");

        // 处理呈现结果
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebuffer_resized)
        {
            LOG_INFO("交换链需要重建（过期、次优或窗口大小改变）");
            m_framebuffer_resized = false;
            recreateSwapChain();
            return VK_ERROR_OUT_OF_DATE_KHR;
        }
        else if (result != VK_SUCCESS)
        {
            LOG_ERROR("呈现交换链图像失败: {}", static_cast<int>(result));
            return result;
        }

        return VK_SUCCESS;
    }

    void RenderSystem::recreateSwapChain()
    {
        auto cpu_profiler = g_runtime_global_context.m_cpu_profiler;

        LOG_INFO("重建交换链...");

        cpu_profiler->beginTimestamp("Recreate Swap Chain");

        // 等待设备空闲
        vkDeviceWaitIdle(m_device->getDevice());

        // 重建交换链
        m_swap_chain->recreateSwapChain();

        // 重建帧缓冲区
        m_pipeline->destroyFramebuffers();
        m_pipeline->createFramebuffers();

        cpu_profiler->endTimestamp("Recreate Swap Chain");

        LOG_INFO("交换链重建完成");
    }

    void RenderSystem::updateCameraAndInput(float dt)
    {
        if (m_input_manager)
        {
            m_input_manager->tick(dt);
        }

        // 更新相机 UBO 数据
        if (m_camera_ubo.isValid() && m_camera)
        {
            struct CameraData
            {
                glm::mat4 view;
                glm::mat4 proj;
            } camera_data {m_camera->getView(), m_camera->getProj()};

            m_camera_ubo.uploadDataDirect(m_device->getDevice(), &camera_data, sizeof(CameraData));
        }
    }
} // namespace Piccolo
