#pragma once
#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/object.h"
#include "runtime/resource/res_type/components/sound.h"
// #include "runtime/resource/res_type/data/sound_data.h"

namespace Piccolo
{
    REFLECTION_TYPE(SoundComponent)
    CLASS(SoundComponent : public Component, WhiteListFields)
    {
        REFLECTION_BODY(SoundComponent)

    public:
        SoundComponent()          = default;
        virtual ~SoundComponent() = default;

        void postLoadResource(std::weak_ptr<GObject> parent_object) override;

        void tick(float delta_time) override;

        const SoundComponentRes& getSoundRes() { return m_sound_res; }

    private:
        META(Enable)
        SoundComponentRes m_sound_res;

        META(Enable)
        std::string m_sound_name;

        META(Enable)
        float m_trigger_dt {0.5};

        float m_total_time {0.0f};
    };
} // namespace Piccolo