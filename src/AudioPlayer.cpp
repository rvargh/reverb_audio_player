#include "AudioPlayer.hpp"

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
    play_audio();
}

void AudioPlayer::pause() {

    if (m_is_playing) {
        m_is_playing = false;
    }
    pAudioClient->Stop();
    std::cout << "Hey I am Paused." << std::endl;
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
    hr = CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, nullptr,CLSCTX_ALL, IID_IMMDeviceEnumerator,reinterpret_cast<void **>(&pEnumerator));

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    // give the default audio endpoint
    hr = pEnumerator->GetDefaultAudioEndpoint(eRender,eConsole,&pDefaultDevice);
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

    hr = pDefaultDevice->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr, reinterpret_cast<void **>(&pAudioClient));
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

    hr = pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED,&pFormat,&ppClosestMatch);

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    auto ext =
    reinterpret_cast<WAVEFORMATEXTENSIBLE*>(ppClosestMatch);

    std::cout << "Valid bits: "
              << ext->Samples.wValidBitsPerSample << '\n';

    std::cout << "SubFormat: "
              << ext->SubFormat.Data1 << '\n';

    std::cout << "Channel mask: "
              << ext->dwChannelMask << '\n';

    formatToUse = (hr == S_FALSE && ppClosestMatch != nullptr)
        ? ppClosestMatch
        : &pFormat;

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,0,
        hnsRequestedDuration,0, formatToUse,nullptr
    );

   // hr = pAudioClient->GetMixFormat(&pFormat);
    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    hr = pAudioClient->GetService(__uuidof(IAudioRenderClient),reinterpret_cast<void **>(&pRenderClient));

    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    pAudioClient->GetBufferSize(&bufferFrameCount);
    hr = pAudioClient->Start();
    if (FAILED(hr)) {
        std::cout << audioClientErrorName(hr) << std::endl;
        std::exit(EXIT_FAILURE);
    }

    hr = pAudioClient->GetService(IID_IAudioClock,reinterpret_cast<void **>(&pAudioClock));
    //std::exit(EXIT_FAILURE);
}

std::string AudioPlayer::audioClientErrorName(const HRESULT hr) {
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

void AudioPlayer::play_audio() {

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

    hnsActualDuration = (double)(REFTIMES_PER_SEC) * bufferFrameCount / formatToUse->nSamplesPerSec;
    UINT64 pu64Position = 0;
    UINT64 pu64QPCPosition = 0;
    UINT64 pu64Frequency = 0;

    hr = pAudioClock->GetPosition(&pu64Position,&pu64QPCPosition);
    std::cout << "pu64Position : " << pu64Position << std::endl;
    std::cout << "pu64Position : " << pu64QPCPosition << std::endl;
   // frames_remaining = total_audio_frames;

    std::cout << "Hey I am from Audio Player clicked play." << std::endl;

    while (frames_remaining > 0 || pause_button_clicked) {
        UINT32 padding = 0;
        hr = pAudioClient->GetCurrentPadding(&padding);
        std::cout << "Remaining Padding " << padding << std::endl;
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        UINT32 available_buffer_count = bufferFrameCount - padding;

        if (available_buffer_count > frames_remaining) {
            available_buffer_count = frames_remaining;
            std::cout << "I am changing the buffer size : " << frames_remaining << std::endl;
        }

        BYTE* pAudioData = nullptr;
        hr = pRenderClient->GetBuffer(available_buffer_count,&pAudioData);
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }

        UINT32 bytesToWrite = available_buffer_count * formatToUse->nBlockAlign;
        std::cout << "Total bytes processed : " << total_byte_written << std::endl;

        // Preferred – work entirely in bytes
        const auto src = static_cast<const BYTE*>(buffer_to_feed);
        std::memcpy(pAudioData, src + total_byte_written, bytesToWrite);
        // printf("Audio data batch: ");
        //  for (size_t j = 0; j < 32; j++) {
        //      printf("%02X ", pAudioData[j]);
        //  }
        //  printf("\n");
        //wave.erase_audio(bytesToWrite,true);

        hr = pRenderClient->ReleaseBuffer(available_buffer_count,0);
        if (FAILED(hr)) {
            std::cout << audioClientErrorName(hr) << std::endl;
            std::exit(EXIT_FAILURE);
        }
        Sleep((DWORD)(hnsActualDuration / REFTIMES_PER_MILLISEC / 4));
        total_byte_written += bytesToWrite;
        frames_remaining -= available_buffer_count;
        hr = pAudioClock->GetPosition(&pu64Position,&pu64QPCPosition);
        hr = pAudioClock->GetFrequency(&pu64Frequency);

        const UINT64 seconds = pu64Position / pu64Frequency;
        std::cout << "After pu64Position : " << seconds << std::endl;
        std::cout << "After pu64Position : " << pu64QPCPosition << std::endl;
        std::cout << "Total bytes processed reduced : " << (total_audio_frames - frames_remaining) << "  bytes to write : " <<  frames_remaining << std::endl;
    }

    std::cout << "Started Playing :" << std::endl;
    pAudioClient->Stop();
    pRenderClient->Release();
    pAudioClient->Release();
    CoUninitialize();
}

