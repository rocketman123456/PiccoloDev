#include "runtime/function/render/profiler/gpu_timestamp.h"

#include "runtime/core/base/macro.h"
#include "runtime/core/log/log_system.h"

#include <cstring>

namespace Piccolo
{
    void GPUTimestampManager::initialize(VkDevice device, VkPhysicalDevice physical_device, uint16_t queries_per_frame, uint16_t max_frames)
    {
        m_device            = device;
        m_queries_per_frame = queries_per_frame;
        m_max_frames        = max_frames;

        // 获取GPU时间戳周期
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physical_device, &properties);
        m_timestamp_period = properties.limits.timestampPeriod;

        // 创建时间戳查询池
        VkQueryPoolCreateInfo query_pool_info {};
        query_pool_info.sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        query_pool_info.queryType  = VK_QUERY_TYPE_TIMESTAMP;
        query_pool_info.queryCount = queries_per_frame * max_frames * 2; // 每个查询需要开始和结束时间戳

        if (vkCreateQueryPool(m_device, &query_pool_info, nullptr, &m_query_pool) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create timestamp query pool!");
            return;
        }

        // 分配时间戳数据缓冲区
        m_timestamps.resize(queries_per_frame * max_frames);
        m_timestamps_data.resize(queries_per_frame * max_frames * 2);

        // m_timestamps      = new GPUTimestamp[queries_per_frame * max_frames];
        // m_timestamps_data = new uint64_t[queries_per_frame * max_frames * 2];

        LOG_INFO("GPU Timestamp Manager initialized with {} queries per frame, {} max frames", queries_per_frame, max_frames);
    }

    void GPUTimestampManager::clear()
    {
        if (m_query_pool != VK_NULL_HANDLE)
        {
            vkDestroyQueryPool(m_device, m_query_pool, nullptr);
            m_query_pool = VK_NULL_HANDLE;
        }

        // delete[] m_timestamps;
        m_timestamps.clear();
        // m_timestamps = nullptr;

        // delete[] m_timestamps_data;
        m_timestamps_data.clear();
        // m_timestamps_data = nullptr;

        m_device                 = VK_NULL_HANDLE;
        m_queries_per_frame      = 0;
        m_max_frames             = 0;
        m_current_query          = 0;
        m_parent_index           = 0;
        m_depth                  = 0;
        m_current_frame_resolved = false;
    }

    bool GPUTimestampManager::hasValidQueries() const { return m_current_query > 0; }

    void GPUTimestampManager::reset()
    {
        m_current_query          = 0;
        m_parent_index           = 0;
        m_depth                  = 0;
        m_current_frame_resolved = false;
    }

    uint32_t GPUTimestampManager::resolve(uint32_t current_frame, GPUTimestamp* timestamps_to_fill)
    {
        if (!hasValidQueries() || m_current_frame_resolved)
        {
            return 0;
        }

        // 从GPU获取时间戳数据
        uint32_t query_count = m_current_query * 2; // 每个查询有开始和结束时间戳
        uint32_t first_query = current_frame * m_queries_per_frame * 2;

        VkResult result = vkGetQueryPoolResults(
            m_device,
            m_query_pool,
            first_query,
            query_count,
            query_count * sizeof(uint64_t),
            m_timestamps_data.data(),
            sizeof(uint64_t),
            VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT
        );

        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to get query pool results!");
            return 0;
        }

        // 转换时间戳数据为GPUTimestamp结构
        uint32_t timestamp_count = 0;
        for (uint32_t i = 0; i < m_current_query; ++i)
        {
            uint32_t timestamp_index = current_frame * m_queries_per_frame + i;
            GPUTimestamp& timestamp = timestamps_to_fill[timestamp_count];
            timestamp.start         = m_timestamps[timestamp_index].start;
            timestamp.end           = m_timestamps[timestamp_index].end;
            timestamp.name          = m_timestamps[timestamp_index].name;
            timestamp.parent_index  = m_timestamps[timestamp_index].parent_index;
            timestamp.depth         = m_timestamps[timestamp_index].depth;
            timestamp.color         = m_timestamps[timestamp_index].color;
            timestamp.frame_index   = current_frame;

            // 计算经过的时间（毫秒）
            uint64_t start_time  = m_timestamps_data[i * 2];
            uint64_t end_time    = m_timestamps_data[i * 2 + 1];
            timestamp.elapsed_ms = (end_time - start_time) * m_timestamp_period / 1000000.0; // 转换为毫秒

            timestamp_count++;
        }

        m_current_frame_resolved = true;
        return timestamp_count;
    }

    uint32_t GPUTimestampManager::push(uint32_t current_frame, const char* name)
    {
        if (m_current_query >= m_queries_per_frame)
        {
            LOG_WARN("Too many timestamp queries for frame {}, ignoring '{}'", current_frame, name);
            return UINT32_MAX;
        }

        uint32_t query_index     = m_current_query;
        uint32_t timestamp_index = current_frame * m_queries_per_frame + query_index;

        // 记录时间戳信息
        m_timestamps[timestamp_index].name         = name;
        m_timestamps[timestamp_index].parent_index = m_parent_index;
        m_timestamps[timestamp_index].depth        = m_depth;
        m_timestamps[timestamp_index].color        = 0xFFFFFFFF; // 默认白色
        m_timestamps[timestamp_index].start        = current_frame * m_queries_per_frame * 2 + query_index * 2;
        m_timestamps[timestamp_index].end          = current_frame * m_queries_per_frame * 2 + query_index * 2 + 1;

        m_parent_index = query_index;
        m_depth++;
        m_current_query++;

        return query_index;
    }

    uint32_t GPUTimestampManager::pop(uint32_t current_frame)
    {
        if (m_current_query == 0)
        {
            LOG_WARN("No timestamp queries to pop for frame {}", current_frame);
            return UINT32_MAX;
        }

        m_current_query--;
        m_depth--;

        // 恢复父级索引
        if (m_current_query > 0)
        {
            m_parent_index = m_timestamps[current_frame * m_queries_per_frame + m_current_query - 1].parent_index;
        }
        else
        {
            m_parent_index = 0;
        }

        return m_current_query;
    }

} // namespace Piccolo
