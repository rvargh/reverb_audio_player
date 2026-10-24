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

void AudioPlayer::load(std::filesystem::path path) {
    // this is where I need to load the mp3 file and play the music
    if (!std::filesystem::exists(path)) {
        std::cerr << "File not found: " << path << std::endl;
        std::exit(EXIT_FAILURE);
    }
    auto size = std::filesystem::file_size(path);

    m_wave.emplace(std::move(path));
    init_system_audio();
}

void AudioPlayer::play() {
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
    stop_worker_thread();
    pAudioClient->Stop();
    // since audio is playing in another thread, need to kill that thread
    std::println("Hey I am Paused and m_is_playing : {}", m_is_playing.load());
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

void AudioPlayer::set_volume(int const value) {
    std::cout << "volume slider moved to : " << value << std::endl;
    m_volume_level = value;
}

void AudioPlayer::init_system_audio() {
    // initialize COM devices
    hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr,CLSCTX_ALL, IID_IMMDeviceEnumerator,
                          reinterpret_cast<void **>(&pEnumerator));

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // give the default audio endpoint
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pEnumerator->Release();

    hr = pDefaultDevice->OpenPropertyStore(STGM_READ, &ppProperties);

    PropVariantInit(&varName);
    if (FAILED(ppProperties->GetValue(PKEY_Device_FriendlyName, &varName))) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    std::wcout << L"Playing through default device: " << varName.pwszVal << std::endl;
    PropVariantClear(&varName);
    ppProperties->Release();

    hr = pDefaultDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&pAudioClient));
    if (FAILED(hr)) {
        std::cout << "Audio device activation failed" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pDefaultDevice->Release();

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    if (m_wave.has_value()) {
        pFormat.wFormatTag = m_wave->audioFormat;
        pFormat.nChannels = m_wave->numChannels;
        pFormat.nSamplesPerSec = m_wave->sampleRate;
        pFormat.nAvgBytesPerSec = m_wave->byteRate;
        pFormat.nBlockAlign = m_wave->blockAlign;
        pFormat.wBitsPerSample = m_wave->bitsPerSample;
        pFormat.cbSize = 0;
    }

    hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &pFormat, &ppClosestMatch);

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    formatToUse = (hr == S_FALSE && ppClosestMatch != nullptr)
                      ? ppClosestMatch
                      : &pFormat;
    // formatToUse->nSamplesPerSec = 44100;

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0,
                                  hnsRequestedDuration, 0, formatToUse, nullptr
    );

    hr = pAudioClient->GetMixFormat(&pFormat1);
    if (FAILED(hr)) {
        std::println("Failed to initialize Audio Client : {}", audioClientErrorName(hr));
        std::exit(EXIT_FAILURE);
    }

    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void **>(&pRenderClient));

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pAudioClient->GetBufferSize(&bufferFrameCount);

    hr = pAudioClient->GetService(IID_IAudioClock, reinterpret_cast<void **>(&pAudioClock));

    // this if it's an 32-bit audio sample
    // need to convert existing 16 to 32
    if (ppClosestMatch->nBlockAlign == 8) {
        m_wave->convert_to_float();
        buffer_to_feed = m_wave->m_float_data.data();
        // A float array size divided by channels/sample size depends on format,
        // but assuming m_wave.m_float_data.size() is total samples:
        // frames = total_samples / num_channels
        total_audio_frames = (m_wave->m_float_data.size() * sizeof(float)) / formatToUse->nBlockAlign;
    } else {
        buffer_to_feed = m_wave->m_audio_data.data();
        total_audio_frames = m_wave->m_audio_data.size() / formatToUse->nBlockAlign;
    }
    frames_remaining = total_audio_frames;
    hnsActualDuration = (double) (REFTIMES_PER_SEC) * bufferFrameCount / formatToUse->nSamplesPerSec;

    //std::exit(EXIT_FAILURE);
}


void AudioPlayer::play_audio() {
    UINT64 pu64Position = 0;
    UINT64 pu64QPCPosition = 0;
    UINT64 pu64Frequency = 0;

    hr = pAudioClock->GetPosition(&pu64Position, &pu64QPCPosition);
    std::cout << "pu64Position : " << pu64Position << std::endl;
    std::cout << "pu64Position : " << pu64QPCPosition << std::endl;
    AudioPlayer::duration_seconds();

    std::cout << "Hey I am from Audio Player clicked play." << std::endl;

    while (frames_remaining > 0 && m_is_playing) {
        UINT32 padding = 0;
        hr = pAudioClient->GetCurrentPadding(&padding);
        //std::cout << "Remaining Padding " << padding << std::endl;
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        UINT32 available_buffer_count = bufferFrameCount - padding;

        if (available_buffer_count > frames_remaining) {
            available_buffer_count = frames_remaining;
            //std::cout << "I am changing the buffer size : " << frames_remaining << std::endl;
        }

        BYTE* pAudioData = nullptr;
        hr = pRenderClient->GetBuffer(available_buffer_count, &pAudioData);
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        UINT32 bytesToWrite = available_buffer_count * formatToUse->nBlockAlign;
        //std::cout << "Total bytes processed : " << total_byte_written << std::endl;

        // Preferred – work entirely in bytes
        const auto src = static_cast<const BYTE *>(buffer_to_feed);
        std::memcpy(pAudioData, src + total_byte_written, bytesToWrite);
        // printf("Audio data batch: ");
        //  for (size_t j = 0; j < 32; j++) {
        //      printf("%02X ", pAudioData[j]);
        //  }
        //  printf("\n");
        //wave.erase_audio(bytesToWrite,true);

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

        const UINT64 seconds = pu64Position / pu64Frequency;
        //AudioPlayer::set_volume(static_cast<int>(seconds));

        //std::println("After pu64Position : {}\n", 123);
        //   std::cout << "After pu64Position : " << seconds << std::endl;
        // std::cout << "After pu64Position : " << seconds << std::endl;
        // std::cout << "After pu64Position : " << pu64QPCPosition << std::endl;
        // std::cout << "Total bytes processed reduced : " << (total_audio_frames - frames_remaining) <<
        //         "  bytes to write : " << frames_remaining << std::endl;
    }

    //std::cout << "Started Playing :" << std::endl;
}

int AudioPlayer::duration_seconds() const {
    return static_cast<int>(total_audio_frames / formatToUse->nSamplesPerSec) / 60;
}

void AudioPlayer::stop_worker_thread() {
    m_is_playing = false;
    if (m_worker.joinable()) {
        std::println("I am closing the worker thread");
        m_worker.join();
    }
}
