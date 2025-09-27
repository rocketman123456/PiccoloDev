#include "runtime/function/sound/sound_manager.h"

#include "runtime/resource/config_manager/config_manager.h"

#include "runtime/core/base/macro.h"

#include <portaudio.h>
#include <sndfile.h>

namespace Piccolo
{
    void SoundManager::initialize()
    {
        PaError err = Pa_Initialize();
        if (err != paNoError)
        {
            LOG_ERROR("Failed to initialize sound");
            LOG_ERROR("PortAudio error: {}", Pa_GetErrorText(err));
            throw std::runtime_error("Pa_Initialize failed");
        }

        int numDevices = Pa_GetDeviceCount();
        if (numDevices < 0)
        {
            LOG_ERROR("ERROR: Pa_CountDevices returned 0x{}", numDevices);
            throw std::runtime_error("Pa_Initialize failed");
        }

        const PaDeviceInfo* deviceInfo;
        for (int i = 0; i < numDevices; i++)
        {
            deviceInfo = Pa_GetDeviceInfo(i);
            LOG_INFO("sound device name: {}", deviceInfo->name);
        }

        PaDeviceIndex dev = Pa_GetDefaultOutputDevice();
        if (dev == paNoDevice)
        {
            Pa_Terminate();
            throw std::runtime_error("No default output device");
        }

        // PaStreamParameters inputParameters;
        // inputParameters.channelCount              = inChan;
        // inputParameters.device                    = inDevNum;
        // inputParameters.hostApiSpecificStreamInfo = NULL;
        // inputParameters.sampleFormat              = paFloat32;
        // inputParameters.suggestedLatency          = Pa_GetDeviceInfo(inDevNum)->defaultLowInputLatency;
        // inputParameters.hostApiSpecificStreamInfo = NULL; // See you specific host's API docs for info on using this field

        PaStreamParameters outParams;
        outParams.device                    = dev;
        outParams.channelCount              = m_channels;
        outParams.sampleFormat              = paFloat32;
        outParams.suggestedLatency          = Pa_GetDeviceInfo(dev)->defaultLowOutputLatency;
        outParams.hostApiSpecificStreamInfo = nullptr;

        err = Pa_OpenStream(&m_stream, nullptr, &outParams, m_sample_rate, paFramesPerBufferUnspecified, paNoFlag, &SoundManager::paCallback, this);
        if (err != paNoError)
        {
            Pa_Terminate();
            throw std::runtime_error("Pa_OpenStream failed");
        }
        err = Pa_StartStream(m_stream);
        if (err != paNoError)
        {
            Pa_CloseStream(m_stream);
            Pa_Terminate();
            throw std::runtime_error("Pa_StartStream failed");
        }
    }

    void SoundManager::clear()
    {
        if (m_stream)
        {
            Pa_StopStream(m_stream);
            Pa_CloseStream(m_stream);
            m_stream = nullptr;
        }
        auto err = Pa_Terminate();
        if (err != paNoError)
        {
            LOG_ERROR("Failed to terminate sound");
            LOG_ERROR("PortAudio error: {}", Pa_GetErrorText(err));
        }
    }

    int SoundManager::loadSound(SoundComponentRes& res)
    {
        auto file_dir = g_runtime_global_context.m_config_manager->getAssetFolder();
        file_dir      = file_dir / res.sound_url;

        SF_INFO  info {};
        SNDFILE* snd = sf_open(file_dir.c_str(), SFM_READ, &info);
        if (!snd)
        {
            LOG_WARN("Failed to open sound: {}", file_dir.c_str());
            return;
        }

        std::vector<float> buffer(info.frames * info.channels);
        sf_readf_float(snd, buffer.data(), info.frames);
        sf_close(snd);

        SoundData data {std::move(buffer), info.channels, info.samplerate};

        int id       = m_next_sound_id++;
        m_sounds[id] = std::move(data);

        res.sound_id = id;

        return id;
    }

    int SoundManager::play(int sound_id, bool loop, float volume)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        it = m_sounds.find(sound_id);
        if (it == m_sounds.end())
        {
            // throw std::runtime_error("Invalid sound_id");
            LOG_WARN("Invalid sound_id {}", sound_id)
            return -1;
        }

        int inst_id          = m_next_instance_id++;
        m_instances[inst_id] = PlayingInstance {&it->second, 0, false, loop, volume};
        return inst_id;
    }

    void SoundManager::pause(int inst_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instances.count(inst_id))
            m_instances[inst_id].paused = true;
    }

    void SoundManager::resume(int inst_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instances.count(inst_id))
            m_instances[inst_id].paused = false;
    }

    void SoundManager::stop(int inst_id)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_instances.erase(inst_id);
    }

    void SoundManager::stopAll()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_instances.clear();
    }

    void SoundManager::setVolume(int inst_id, float vol)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instances.count(inst_id))
            m_instances[inst_id].volume = vol;
    }

    void SoundManager::tick(double dt)
    {
        //
    }
} // namespace Piccolo