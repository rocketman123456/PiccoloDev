#pragma once

#include <memory>
#include <string>

namespace Piccolo
{
    class LogSystem;
    class FileSystem;
    class ConfigManager;

    class AssetManager;

    class EventSystem;

    class WindowSystem;
    class RenderSystem;

    class CPUProfiler;
    class GPUProfiler;

    struct EngineInitParams;

    /// Manage the lifetime and creation/destruction order of all global system
    class RuntimeGlobalContext
    {
    public:
        // create all global systems and initialize these systems
        void startSystems(const std::string& config_file_path);
        // destroy all global systems
        void shutdownSystems();

    public:
        std::shared_ptr<ConfigManager> m_config_manager;
        std::shared_ptr<FileSystem>    m_file_system;
        std::shared_ptr<LogSystem>     m_logger_system;

        std::shared_ptr<AssetManager> m_asset_manager;

        std::shared_ptr<EventSystem> m_event_system;

        std::shared_ptr<WindowSystem> m_window_system;
        std::shared_ptr<RenderSystem> m_render_system;

        std::shared_ptr<CPUProfiler> m_cpu_profiler;
        std::shared_ptr<GPUProfiler> m_gpu_profiler;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Piccolo