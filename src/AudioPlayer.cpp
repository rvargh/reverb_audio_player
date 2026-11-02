#include "AudioPlayer.hpp"

AudioPlayer::~AudioPlayer() {
    if (m_worker.joinable()) {
        m_worker.join();
    }
    pAudioClient->Stop();
    pRenderClient->Release();
    pAudioClient->Release();
    CoUninitialize();
}

AudioPlayer::AudioPlayer(const PlayList &play_list) : m_play_list(play_list) {
    init_system_audio();
    if (m_play_list.total_tracks() > 0) {
        m_current_index = 0;
        load_audio_from_playlist();
    }
}

void AudioPlayer::play() {

    if (frames_remaining == 0) {
        load_audio_from_playlist();
    }
    if (!m_is_playing) {
        m_is_playing = true;
    }
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::println("Failed to start Audio Client : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }
    m_worker = std::thread(&AudioPlayer::play_audio, this);
}

void AudioPlayer::pause() {
    if (m_is_playing) {
        m_is_playing = false;
    }
    stop_audio_client();
    stop_worker_thread();
}

void AudioPlayer::previous() {
    std::cout << "Hey I am from Audio Player clicked previous." << std::endl;
}

void AudioPlayer::next() {
    std::cout << "Hey I am from Audio Player clicked next." << std::endl;
}

bool AudioPlayer::is_playing() const {
    return m_is_playing;
}

void AudioPlayer::init_system_audio() {
    // initialize COM devices
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr,CLSCTX_ALL, IID_IMMDeviceEnumerator,
                          reinterpret_cast<void **>(&pEnumerator));

    if (FAILED(hr)) {
        std::println("Failed to initialize  COM devices : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }
    // give the default audio endpoint
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
    if (FAILED(hr)) {
        std::println("Failed to retrieve default audio endpoints : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }
    pEnumerator->Release();

    hr = pDefaultDevice->OpenPropertyStore(STGM_READ, &ppProperties);
    PropVariantInit(&varName);
    if (FAILED(ppProperties->GetValue(PKEY_Device_FriendlyName, &varName))) {
        std::println("Failed to retrieve default device name : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }

    PropVariantClear(&varName);
    ppProperties->Release();

    //hr = pAudioClient->GetMixFormat(&pFormat1);

    // hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, pFormat1, &ppClosestMatch);
    // if (FAILED(hr)) {
    //     std::println("Current Audio Format is not supported : {}", audioClientErrorName(hr));
    //     std::exit(EXIT_FAILURE);
    // }

    // formatToUse = (hr == S_FALSE && ppClosestMatch != nullptr)
    //                   ? ppClosestMatch
    //                   : &pFormat;
}


void AudioPlayer::play_audio() {
    UINT64 pu64Position = 0;
    UINT64 pu64QPCPosition = 0;
    UINT64 pu64Frequency = 0;

    hr = pAudioClock->GetPosition(&pu64Position, &pu64QPCPosition);

    while (frames_remaining > 0 && m_is_playing) {
        UINT32 padding = 0;
        hr = pAudioClient->GetCurrentPadding(&padding);
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        UINT32 available_buffer_count = bufferFrameCount - padding;
        if (available_buffer_count > frames_remaining) {
            available_buffer_count = frames_remaining;
        }

        BYTE* pAudioData = nullptr;
        hr = pRenderClient->GetBuffer(available_buffer_count, &pAudioData);
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }
        UINT32 bytesToWrite = available_buffer_count * pFormat.nBlockAlign;

        // Preferred – work entirely in bytes
        const auto src = static_cast<const BYTE *>(buffer_to_feed);
        std::memcpy(pAudioData, src + total_byte_written, bytesToWrite);

        hr = pRenderClient->ReleaseBuffer(available_buffer_count, 0);
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }
        Sleep((DWORD) (hnsActualDuration / REFTIMES_PER_MILLISEC / 4));
        total_byte_written += bytesToWrite;
        frames_remaining -= available_buffer_count;
        hr = pAudioClock->GetPosition(&pu64Position, &pu64QPCPosition);
        hr = pAudioClock->GetFrequency(&pu64Frequency);
        m_position_seconds = pu64Position / pu64Frequency;
    }

    if (frames_remaining > 0) {
        m_is_playing = false;
        pAudioClient->Stop();
    }
}

int AudioPlayer::duration_seconds() const {
    return static_cast<int>(total_audio_frames / pFormat.nSamplesPerSec);
}

void AudioPlayer::stop_worker_thread() {
    m_is_playing = false;
    if (m_worker.joinable()) {
        std::println("I am closing the worker thread");
        m_worker.join();
    }
}

int AudioPlayer::current_second() const {
    return static_cast<int>(total_byte_written / pFormat.nBlockAlign / pFormat.nSamplesPerSec);
}

void AudioPlayer::change_volume(const int value) {
    m_volume_level = value;
    const float level = std::clamp(value / 100.0f, 0.0f, 1.0f);
    hr = ISimpleAudioVolume->SetMasterVolume(level, nullptr);
}

void AudioPlayer::stop_audio_client() const {
    pAudioClient->Stop();
}

void AudioPlayer::seek_player(const int value) {
    // change the ul seek bar as well
    m_position_seconds = value;
    // convert seconds to frames
    UINT64 targetFrame = value * pFormat.nSamplesPerSec;
    //calculate the byte offset
    UINT64 byteOffset = targetFrame * pFormat.nBlockAlign;

    std::println("Bytes bytesOffset : {}", byteOffset);
    std::println("Current bytes passed : {} ", total_byte_written);
    // update the value of total_byte_written
    total_byte_written = byteOffset;
}

void AudioPlayer::load_audio_from_playlist() {

    const Track &current_track = m_play_list.get_track(0);
    m_wave.emplace(current_track.file_path);

    if (m_wave.has_value()) {
        pFormat.wFormatTag = m_wave->audioFormat;
        pFormat.nChannels = m_wave->numChannels;
        pFormat.nSamplesPerSec = m_wave->sampleRate;
        pFormat.nAvgBytesPerSec = m_wave->byteRate;
        pFormat.nBlockAlign = m_wave->blockAlign;
        pFormat.wBitsPerSample = m_wave->bitsPerSample;
        pFormat.cbSize = 0;
    }

    hr = pDefaultDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&pAudioClient));
    if (FAILED(hr)) {
        std::println("Failed to Activate Audio Client : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }
    pDefaultDevice->Release();

    hr = pAudioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        hnsRequestedDuration, 0, &pFormat, nullptr
    );
    if (FAILED(hr)) {
        std::println("Failed to initialize Audio Client : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }

    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void **>(&pRenderClient));
    if (FAILED(hr)) {
        std::println("Failed to initialize  IAudioRenderClient : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }

    hr = pAudioClient->GetService(IID_ISimpleAudioVolume, reinterpret_cast<void **>(&ISimpleAudioVolume));
    if (FAILED(hr)) {
        std::println("Failed to initialize  ISimpleAudioVolume: {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }

    pAudioClient->GetBufferSize(&bufferFrameCount);
    hr = pAudioClient->GetService(IID_IAudioClock, reinterpret_cast<void **>(&pAudioClock));


    // this if it's an 32-bit audio sample
    // need to convert existing 16 to 32
    if (pFormat.nBlockAlign == 8) {
        m_wave->convert_to_float();
        buffer_to_feed = m_wave->m_float_data.data();
        // A float array size divided by channels/sample size depends on format,
        // but assuming m_wave.m_float_data.size() is total samples:
        // frames = total_samples / num_channels
        total_audio_frames = (m_wave->m_float_data.size() * sizeof(float)) / pFormat.nBlockAlign;
    } else {
        buffer_to_feed = m_wave->m_audio_data.data();
        total_audio_frames = (m_wave->m_audio_data.size() * sizeof(short)) / pFormat.nBlockAlign;
    }
    frames_remaining = total_audio_frames;
    hnsActualDuration = (double) (REFTIMES_PER_SEC) * bufferFrameCount / pFormat.nSamplesPerSec;
}

const Track& AudioPlayer::get_current_track_info() const {
    return m_play_list.get_track(0);
}

int AudioPlayer::total_track() const {
    return m_play_list.total_tracks();
}

int AudioPlayer::current_playlist_index() const {
    return m_current_index;
}
