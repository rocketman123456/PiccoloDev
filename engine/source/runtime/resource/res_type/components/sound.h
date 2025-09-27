#pragma once
#include "runtime/core/meta/reflection/reflection.h"
// #include "runtime/resource/res_type/data/sound_data.h"

#include <string>

namespace Piccolo
{
    REFLECTION_TYPE(SoundComponentRes)
    CLASS(SoundComponentRes, WhiteListFields)
    {
        REFLECTION_BODY(SoundComponentRes);

    public:
        SoundComponentRes()  = default;
        ~SoundComponentRes() = default;

        META(Enable)
        std::string sound_url;

        int sound_id {-1};

        // int channels {2};
        // int sample_rate {44100};

        // std::vector<float> samples;

        // size_t position = 0; // current frame index
        // bool   paused   = false;
        // bool   loop     = false;
        // float  volume   = 1.0f;
    };
} // namespace Piccolo
