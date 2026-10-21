#pragma once

#include "Common.hpp"
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#include <audioclient.h>
#include "Wave.hpp"

class AudioPlayer {

    public:
        void load(std::filesystem::path path);
        void init_system_audio();
        void play();
        void next();
        void previous();
        bool is_playing() const;
        void pause();
        // void stop();
        void set_volume(int value);
        // void position_changed();
        static std::string audioClientErrorName(HRESULT hr);
        void play_audio();

    private:
        bool m_is_playing = false;
        int m_volume_level = 0;

    public:
        HRESULT hr;
        REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
        REFERENCE_TIME hnsActualDuration;
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDevice* pDefaultDevice = nullptr;
        UINT pcDevices;
        IPropertyStore* ppProperties = nullptr;
        const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
        const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
        IAudioClient* pAudioClient = nullptr;
        void* buffer_to_feed;
        PROPVARIANT varName;
        WAVEFORMATEX pFormat = {};
        WAVEFORMATEX* ppClosestMatch = nullptr;
        IAudioRenderClient* pRenderClient = nullptr;
        UINT32 bufferFrameCount = 0;
        size_t total_audio_frames = 0;
        size_t total_byte_written = 0;
        size_t frames_remaining = 0;
        const IID IID_IAudioClock = __uuidof(IAudioClock);
        IAudioClock* pAudioClock = nullptr;
        WAVEFORMATEX* formatToUse;
        std::optional<Wave> m_wave;
        bool pause_button_clicked = false;
};