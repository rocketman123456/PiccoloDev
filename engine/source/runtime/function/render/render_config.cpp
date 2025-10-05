#include "runtime/function/render/render_config.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/log/log_system.h"

#include <fstream>
#include <sstream>

namespace Piccolo
{
    // ========== RenderConfig 实现 ==========

    RenderQuality RenderConfig::getQualityLevel() const
    {
        // 根据当前配置判断质量等级
        if (pipeline.enable_fxaa && lighting.shadow_map_size >= 4096 && performance.max_meshes >= 8192) {
            return RenderQuality::ULTRA;
        } else if (pipeline.enable_fxaa && lighting.shadow_map_size >= 2048 && performance.max_meshes >= 4096) {
            return RenderQuality::HIGH;
        } else if (lighting.shadow_map_size >= 1024 && performance.max_meshes >= 2048) {
            return RenderQuality::MEDIUM;
        } else {
            return RenderQuality::LOW;
        }
    }

    void RenderConfig::setQualityLevel(RenderQuality quality)
    {
        switch (quality) {
            case RenderQuality::LOW:
                pipeline.enable_fxaa = false;
                pipeline.enable_ssao = false;
                pipeline.enable_ssr = false;
                lighting.shadow_map_size = 512;
                lighting.max_directional_lights = 2;
                lighting.max_point_lights = 16;
                performance.max_meshes = 1024;
                performance.max_materials = 256;
                break;

            case RenderQuality::MEDIUM:
                pipeline.enable_fxaa = false;
                pipeline.enable_ssao = false;
                pipeline.enable_ssr = false;
                lighting.shadow_map_size = 1024;
                lighting.max_directional_lights = 3;
                lighting.max_point_lights = 32;
                performance.max_meshes = 2048;
                performance.max_materials = 512;
                break;

            case RenderQuality::HIGH:
                pipeline.enable_fxaa = true;
                pipeline.enable_ssao = true;
                pipeline.enable_ssr = false;
                lighting.shadow_map_size = 2048;
                lighting.max_directional_lights = 4;
                lighting.max_point_lights = 64;
                performance.max_meshes = 4096;
                performance.max_materials = 1024;
                break;

            case RenderQuality::ULTRA:
                pipeline.enable_fxaa = true;
                pipeline.enable_ssao = true;
                pipeline.enable_ssr = true;
                lighting.shadow_map_size = 4096;
                lighting.max_directional_lights = 4;
                lighting.max_point_lights = 128;
                performance.max_meshes = 8192;
                performance.max_materials = 2048;
                break;

            case RenderQuality::CUSTOM:
                // 自定义质量，不修改任何设置
                break;
        }
    }

    bool RenderConfig::isValid() const
    {
        // 验证基本范围
        if (lighting.shadow_map_size < 256 || lighting.shadow_map_size > 8192) {
            return false;
        }
        if (lighting.max_directional_lights > 8) {
            return false;
        }
        if (lighting.max_point_lights > 256) {
            return false;
        }
        if (performance.max_meshes < 100 || performance.max_meshes > 16384) {
            return false;
        }
        if (display.window_width < 640 || display.window_height < 480) {
            return false;
        }
        if (display.target_fps < 30 || display.target_fps > 240) {
            return false;
        }
        return true;
    }

    void RenderConfig::resetToDefaults()
    {
        // 重置为默认配置
        pipeline = {};
        lighting = {};
        post_process = {};
        performance = {};
        debug = {};
        display = {};

        // 设置默认值
        pipeline.pipeline_type = RENDER_PIPELINE_TYPE::DEFERRED_PIPELINE;
        lighting.shadow_map_size = 2048;
        lighting.max_directional_lights = 4;
        lighting.max_point_lights = 64;
        performance.max_meshes = 4096;
        performance.max_materials = 1024;
        display.window_width = 1920;
        display.window_height = 1080;
        display.target_fps = 60;
    }

    // ========== RenderConfigManager 实现 ==========

    bool RenderConfigManager::loadConfig(const std::string& file_path)
    {
        LOG_INFO("加载渲染配置: {}", file_path);

        std::ifstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR("无法打开配置文件: {}", file_path);
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        if (!loadFromJson(buffer.str())) {
            LOG_ERROR("配置文件格式错误: {}", file_path);
            return false;
        }

        if (!validateConfig()) {
            LOG_ERROR("配置验证失败，使用默认配置");
            m_config.resetToDefaults();
            return false;
        }

        LOG_INFO("渲染配置加载成功");
        return true;
    }

    bool RenderConfigManager::saveConfig(const std::string& file_path) const
    {
        LOG_INFO("保存渲染配置: {}", file_path);

        if (!validateConfig()) {
            LOG_ERROR("配置验证失败，无法保存");
            return false;
        }

        std::ofstream file(file_path);
        if (!file.is_open()) {
            LOG_ERROR("无法创建配置文件: {}", file_path);
            return false;
        }

        std::string json_str = saveToJson();
        file << json_str;
        file.close();

        LOG_INFO("渲染配置保存成功");
        return true;
    }

    void RenderConfigManager::applyLowQualityPreset()
    {
        LOG_INFO("应用低质量预设");
        m_config.setQualityLevel(RenderQuality::LOW);
        m_config.performance.enable_instancing = false;
        m_config.performance.enable_frustum_culling = true;
        m_config.performance.enable_occlusion_culling = false;
    }

    void RenderConfigManager::applyMediumQualityPreset()
    {
        LOG_INFO("应用中等质量预设");
        m_config.setQualityLevel(RenderQuality::MEDIUM);
        m_config.performance.enable_instancing = true;
        m_config.performance.enable_frustum_culling = true;
        m_config.performance.enable_occlusion_culling = false;
    }

    void RenderConfigManager::applyHighQualityPreset()
    {
        LOG_INFO("应用高质量预设");
        m_config.setQualityLevel(RenderQuality::HIGH);
        m_config.performance.enable_instancing = true;
        m_config.performance.enable_frustum_culling = true;
        m_config.performance.enable_occlusion_culling = true;
    }

    void RenderConfigManager::applyUltraQualityPreset()
    {
        LOG_INFO("应用超高质量预设");
        m_config.setQualityLevel(RenderQuality::ULTRA);
        m_config.performance.enable_instancing = true;
        m_config.performance.enable_frustum_culling = true;
        m_config.performance.enable_occlusion_culling = true;
        m_config.pipeline.enable_temporal_aa = true;
    }

    void RenderConfigManager::applyPerformancePreset()
    {
        LOG_INFO("应用性能优化预设");
        m_config.setQualityLevel(RenderQuality::LOW);
        m_config.performance.enable_instancing = true;
        m_config.performance.enable_frustum_culling = true;
        m_config.performance.enable_occlusion_culling = true;
        m_config.display.vsync = false;
        m_config.display.target_fps = 120;
    }

    void RenderConfigManager::applyQualityPreset()
    {
        LOG_INFO("应用质量优先预设");
        m_config.setQualityLevel(RenderQuality::ULTRA);
        m_config.performance.enable_instancing = true;
        m_config.performance.enable_frustum_culling = true;
        m_config.performance.enable_occlusion_culling = true;
        m_config.pipeline.enable_temporal_aa = true;
        m_config.display.vsync = true;
        m_config.display.target_fps = 60;
    }

    bool RenderConfigManager::loadFromJson(const std::string& /* json_str */)
    {
        // 这里应该使用JSON解析库（如nlohmann/json）来解析JSON
        // 为了简化，这里只做基本的字符串解析示例
        
        // TODO: 实现完整的JSON解析
        // 目前只是占位符实现
        LOG_INFO("JSON解析功能尚未完全实现，使用默认配置");
        m_config.resetToDefaults();
        return true;
    }

    std::string RenderConfigManager::saveToJson() const
    {
        // 这里应该使用JSON序列化库来生成JSON
        // 为了简化，这里只返回基本的JSON结构
        
        std::stringstream json;
        json << "{\n";
        json << "  \"pipeline\": {\n";
        json << "    \"pipeline_type\": " << static_cast<int>(m_config.pipeline.pipeline_type) << ",\n";
        json << "    \"enable_fxaa\": " << (m_config.pipeline.enable_fxaa ? "true" : "false") << ",\n";
        json << "    \"enable_ssao\": " << (m_config.pipeline.enable_ssao ? "true" : "false") << ",\n";
        json << "    \"enable_ssr\": " << (m_config.pipeline.enable_ssr ? "true" : "false") << "\n";
        json << "  },\n";
        json << "  \"lighting\": {\n";
        json << "    \"shadow_map_size\": " << m_config.lighting.shadow_map_size << ",\n";
        json << "    \"max_directional_lights\": " << m_config.lighting.max_directional_lights << ",\n";
        json << "    \"max_point_lights\": " << m_config.lighting.max_point_lights << "\n";
        json << "  },\n";
        json << "  \"display\": {\n";
        json << "    \"window_width\": " << m_config.display.window_width << ",\n";
        json << "    \"window_height\": " << m_config.display.window_height << ",\n";
        json << "    \"vsync\": " << (m_config.display.vsync ? "true" : "false") << "\n";
        json << "  }\n";
        json << "}\n";
        
        return json.str();
    }

    bool RenderConfigManager::validateConfig() const
    {
        return m_config.isValid();
    }
} // namespace Piccolo
