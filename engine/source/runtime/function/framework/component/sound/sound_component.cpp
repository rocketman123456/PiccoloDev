#include "runtime/function/framework/component/sound/sound_component.h"

#include "runtime/engine.h"

#include "runtime/core/base/macro.h"

#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/sound/sound_manager.h"

namespace Piccolo
{
    void SoundComponent::postLoadResource(std::weak_ptr<GObject> parent_object)
    {
        m_parent_object = parent_object;

        LOG_DEBUG("sound_url: {}", m_sound_res.sound_url);

        g_runtime_global_context.m_sound_manager->loadSound(m_sound_res);
    }

    void SoundComponent::tick(float delta_time)
    {
        if ((m_tick_in_editor_mode == false) && g_is_editor_mode)
            return;

        m_total_time += delta_time;

        if (m_total_time > m_trigger_dt)
        {
            int inst_id = g_runtime_global_context.m_sound_manager->play(m_sound_res.sound_id);
            m_total_time = 0.0;
        }
    }
} // namespace Piccolo