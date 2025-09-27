#include "runtime/function/global/global_context.h"

#include "runtime/engine.h"

#include "runtime/core/log/log_system.h"
#include "runtime/platform/file_service/file_service.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/function/render/window_system.h"

namespace Piccolo
{
    RuntimeGlobalContext g_runtime_global_context;

    void RuntimeGlobalContext::startSystems(const std::string& config_file_path)
    {
        m_config_manager = std::make_shared<ConfigManager>();
        m_config_manager->initialize(config_file_path);

        m_file_system = std::make_shared<FileSystem>();
        m_logger_system = std::make_shared<LogSystem>();
        m_asset_manager = std::make_shared<AssetManager>();

        m_window_system = std::make_shared<WindowSystem>();
        WindowCreateInfo window_create_info;
        m_window_system->initialize(window_create_info);
    }

    void RuntimeGlobalContext::shutdownSystems()
    {
        m_window_system.reset();

        m_asset_manager.reset();
        m_logger_system.reset();
        m_file_system.reset();
        m_config_manager.reset();
    }
} // namespace Piccolo