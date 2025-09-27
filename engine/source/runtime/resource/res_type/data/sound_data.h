#pragma once
#include "runtime/core/meta/reflection/reflection.h"

#include <vector>

namespace Piccolo
{
    REFLECTION_TYPE(SoundRes)
    CLASS(SoundRes, Fields)
    {
        REFLECTION_BODY(SoundRes);

    public:
        std::vector<float> samples;
        int                channels;
        int                sample_rate;
    };
} // namespace Piccolo
