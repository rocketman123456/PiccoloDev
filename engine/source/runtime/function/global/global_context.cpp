#include "runtime/function/global/global_context.h"

#include "runtime/engine.h"

#include "runtime/core/log/log_system.h"
#include "runtime/platform/file_service/file_service.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/event/event_system.h"

#include "runtime/function/render/gpu_device.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

#include "runtime/function/ecs/coordinator.h"

#include "runtime/function/profiler/cpu_profiler.h"
#include "runtime/function/profiler/gpu_profiler.h"

namespace Piccolo
{
    RuntimeGlobalContext g_runtime_global_context;

    void RuntimeGlobalContext::startSystems(const std::string& config_file_path)
    {
        m_config_manager = std::make_shared<ConfigManager>();
        m_config_manager->initialize(config_file_path);

        m_file_system   = std::make_shared<FileSystem>();
        m_logger_system = std::make_shared<LogSystem>();

        m_asset_manager = std::make_shared<AssetManager>();

        m_event_system = std::make_shared<EventSystem>();
        m_event_system->initialize();

        m_window_system = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info;
        m_window_system->initialize(window_create_info);

        m_render_system = std::make_shared<RenderSystem>();
        m_render_system->initialize();

        m_cpu_profiler = std::make_shared<CPUProfiler>();
        m_cpu_profiler->initialize();
        m_gpu_profiler = std::make_shared<GPUProfiler>();
        {
            auto device = m_render_system->getDevice();
            m_gpu_profiler->initialize(device->getDevice(), device->getPhysicalDevice(), 64, 3); // 每帧64个查询，最多3帧
        }
    }

    void RuntimeGlobalContext::shutdownSystems()
    {
        m_gpu_profiler.reset();
        m_cpu_profiler.reset();

        m_render_system->clear();
        m_render_system.reset();
        m_window_system.reset();

        m_event_system->clear();
        m_event_system.reset();

        m_asset_manager.reset();

        m_logger_system.reset();
        m_file_system.reset();
        m_config_manager.reset();
    }
} // namespace Piccolo