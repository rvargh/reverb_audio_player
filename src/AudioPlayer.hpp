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
        void play_audio();

        int current_second() const;

        int duration_seconds() const;

        static std::string audioClientErrorName(const HRESULT hr) {
            switch (hr) {
                case AUDCLNT_E_NOT_INITIALIZED: return "AUDCLNT_E_NOT_INITIALIZED";
                case AUDCLNT_E_ALREADY_INITIALIZED: return "AUDCLNT_E_ALREADY_INITIALIZED";
                case AUDCLNT_E_WRONG_ENDPOINT_TYPE: return "AUDCLNT_E_WRONG_ENDPOINT_TYPE";
                case AUDCLNT_E_DEVICE_INVALIDATED: return "AUDCLNT_E_DEVICE_INVALIDATED";
                case AUDCLNT_E_NOT_STOPPED: return "AUDCLNT_E_NOT_STOPPED";
                case AUDCLNT_E_BUFFER_TOO_LARGE: return "AUDCLNT_E_BUFFER_TOO_LARGE";
                case AUDCLNT_E_OUT_OF_ORDER: return "AUDCLNT_E_OUT_OF_ORDER";
                case AUDCLNT_E_UNSUPPORTED_FORMAT: return "AUDCLNT_E_UNSUPPORTED_FORMAT";
                case AUDCLNT_E_INVALID_SIZE: return "AUDCLNT_E_INVALID_SIZE";
                case AUDCLNT_E_DEVICE_IN_USE: return "AUDCLNT_E_DEVICE_IN_USE";
                case AUDCLNT_E_BUFFER_OPERATION_PENDING: return "AUDCLNT_E_BUFFER_OPERATION_PENDING";
                case AUDCLNT_E_THREAD_NOT_REGISTERED: return "AUDCLNT_E_THREAD_NOT_REGISTERED";
                case AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED: return "AUDCLNT_E_EXCLUSIVE_MODE_NOT_ALLOWED";
                case AUDCLNT_E_ENDPOINT_CREATE_FAILED: return "AUDCLNT_E_ENDPOINT_CREATE_FAILED";
                case AUDCLNT_E_SERVICE_NOT_RUNNING: return "AUDCLNT_E_SERVICE_NOT_RUNNING";
                case AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED: return "AUDCLNT_E_EVENTHANDLE_NOT_EXPECTED";
                case AUDCLNT_E_EXCLUSIVE_MODE_ONLY: return "AUDCLNT_E_EXCLUSIVE_MODE_ONLY";
                case AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL: return "AUDCLNT_E_BUFDURATION_PERIOD_NOT_EQUAL";
                case AUDCLNT_E_EVENTHANDLE_NOT_SET: return "AUDCLNT_E_EVENTHANDLE_NOT_SET";
                case AUDCLNT_E_INCORRECT_BUFFER_SIZE: return "AUDCLNT_E_INCORRECT_BUFFER_SIZE";
                case AUDCLNT_E_BUFFER_SIZE_ERROR: return "AUDCLNT_E_BUFFER_SIZE_ERROR";
                case AUDCLNT_E_CPUUSAGE_EXCEEDED: return "AUDCLNT_E_CPUUSAGE_EXCEEDED";
                case AUDCLNT_E_BUFFER_ERROR: return "AUDCLNT_E_BUFFER_ERROR";
                case AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED: return "AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED";
                case AUDCLNT_E_INVALID_DEVICE_PERIOD: return "AUDCLNT_E_INVALID_DEVICE_PERIOD";
                default: return "INVALID_ERROR_CODES";
            }
        }

        void stop_worker_thread();

    private:
        int m_volume_level = 0;

    public:
        HRESULT hr;
        REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
        REFERENCE_TIME hnsActualDuration;
        IMMDeviceEnumerator* pEnumerator = nullptr;
        IMMDevice* pDefaultDevice = nullptr;
        UINT pcDevices;
        IPropertyStore* ppProperties = nullptr;
        PROPVARIANT varName;
        WAVEFORMATEX pFormat{};
        WAVEFORMATEX* pFormat1 = {};
        WAVEFORMATEX* ppClosestMatch = nullptr;
        IAudioRenderClient* pRenderClient = nullptr;
        const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
        const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
        const IID IID_IAudioClock = __uuidof(IAudioClock);
        IAudioClient* pAudioClient = nullptr;
        IAudioClock* pAudioClock = nullptr;
        WAVEFORMATEX* formatToUse;
        void* buffer_to_feed;
        UINT32 bufferFrameCount = 0;
        size_t total_audio_frames = 0;
        size_t total_byte_written = 0;
        size_t frames_remaining = 0;
        std::optional<Wave> m_wave;
        std::thread m_worker;
        std::atomic<bool> m_is_playing{false};
        UINT64 m_position_seconds{0};
        ~AudioPlayer();
};
