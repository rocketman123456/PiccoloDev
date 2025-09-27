#pragma once
#include "runtime/function/framework/component/sound/sound_component.h"
#include "runtime/resource/res_type/components/sound.h"

#include <portaudio.h>
#include <sndfile.h>

#include <atomic>
#include <cmath>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Piccolo
{
    struct SoundData
    {
        std::vector<float> samples;

        int channels {2};
        int sample_rate {44100};

        size_t frames() const { return samples.empty() ? 0 : samples.size() / channels; }
    };

    struct PlayingInstance
    {
        // std::shared_ptr<const SoundData> sound;
        const SoundData* sound;

        size_t position = 0; // current frame index
        bool   paused   = false;
        bool   loop     = false;
        float  volume   = 1.0f;
    };

    class SoundManager
    {
    public:
        SoundManager()  = default;
        ~SoundManager() = default;

        SoundManager(const SoundManager&)            = delete;
        SoundManager& operator=(const SoundManager&) = delete;

        void initialize();
        void clear();
        void tick(double dt);

        int  loadSound(SoundComponentRes& res);
        int  play(int sound_id, bool loop = false, float volume = 1.0f);
        void pause(int instId);
        void resume(int instId);
        void stop(int instId);
        void stopAll();
        void setVolume(int instId, float vol);

    protected:
        static int paCallback(const void* input, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData)
        {
            auto*  engine = reinterpret_cast<SoundManager*>(userData);
            float* out    = reinterpret_cast<float*>(output);
            std::fill(out, out + frameCount * engine->m_channels, 0.0f);

            std::lock_guard<std::mutex> lock(engine->m_mutex);
            for (auto it = engine->m_instances.begin(); it != engine->m_instances.end();)
            {
                auto& inst = it->second;
                if (inst.paused)
                {
                    ++it;
                    continue;
                }

                const SoundData* snd = inst.sound;
                for (unsigned long i = 0; i < frameCount; i++)
                {
                    if (inst.position >= snd->samples.size() / snd->channels)
                    {
                        if (inst.loop)
                        {
                            inst.position = 0;
                        }
                        else
                        {
                            it = engine->m_instances.erase(it);
                            // break;
                            goto NextInstance;
                        }
                    }
                    for (int ch = 0; ch < engine->m_channels; ch++)
                    {
                        float sample = 0.0f;
                        if (ch < snd->channels)
                        {
                            sample = snd->samples[inst.position * snd->channels + ch];
                        }
                        out[i * engine->m_channels + ch] += sample * inst.volume;
                    }
                    inst.position++;
                }
                ++it;
                NextInstance:;
            }
            return paContinue;
        }

    private:
        PaStream* m_stream {nullptr};

        int m_channels    = 2;
        int m_sample_rate = 44100;

        std::unordered_map<int, SoundData>       m_sounds;
        std::unordered_map<int, PlayingInstance> m_instances;

        std::mutex       m_mutex;
        std::atomic<int> m_next_sound_id {1};
        std::atomic<int> m_next_instance_id {1};
    };
} // namespace Piccolo
