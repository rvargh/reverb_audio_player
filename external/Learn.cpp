#include "AudioPlayer.hpp"

#include <algorithm>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <print>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;

namespace {

void throw_if_failed(HRESULT hr, const char* what) {
    if (FAILED(hr)) {
        throw std::runtime_error(std::string(what) + " (HRESULT 0x" +
                                 std::format("{:08X}", static_cast<unsigned>(hr)) + ")");
    }
}

// int16 interleaved -> float32 interleaved, remap channels, resample (linear).
std::vector<float> convert_to_device(const std::vector<short>& src,
                                     uint32_t src_rate, uint16_t src_ch,
                                     uint32_t dst_rate, uint16_t dst_ch) {
    if (src_ch == 0 || src_rate == 0) throw std::runtime_error("Invalid source format");

    const size_t src_frames = src.size() / src_ch;
    if (src_frames == 0) return {};

    // 1) int16 -> float, fix channel count (mono is duplicated, extras dropped)
    std::vector<float> tmp(src_frames * dst_ch);
    for (size_t f = 0; f < src_frames; ++f) {
        for (uint16_t c = 0; c < dst_ch; ++c) {
            const uint16_t sc = std::min<uint16_t>(c, static_cast<uint16_t>(src_ch - 1));
            tmp[f * dst_ch + c] = src[f * src_ch + sc] / 32768.0f;
        }
    }
    if (src_rate == dst_rate) return tmp;

    // 2) linear resample. Swap in soxr / libsamplerate later for better quality.
    const double ratio = static_cast<double>(src_rate) / dst_rate;
    const size_t dst_frames = static_cast<size_t>(src_frames / ratio);
    std::vector<float> out(dst_frames * dst_ch);
    for (size_t f = 0; f < dst_frames; ++f) {
        const double pos = f * ratio;
        const size_t i0 = static_cast<size_t>(pos);
        const size_t i1 = std::min(i0 + 1, src_frames - 1);
        const float t = static_cast<float>(pos - i0);
        for (uint16_t c = 0; c < dst_ch; ++c) {
            out[f * dst_ch + c] = tmp[i0 * dst_ch + c] * (1.0f - t) + tmp[i1 * dst_ch + c] * t;
        }
    }
    return out;
}

} // namespace

// ---------------------------------------------------------------- lifetime

AudioPlayer::AudioPlayer(const PlayList& play_list) : m_play_list(play_list) {
    init_system_audio();
    if (m_play_list.total_tracks() > 0) {
        load_track(0);
    }
}

AudioPlayer::~AudioPlayer() {
    stop_worker_thread();
    if (m_client) m_client->Stop();
    // ComPtr members release themselves, but they must go BEFORE CoUninitialize.
    m_volume.Reset();
    m_render.Reset();
    m_client.Reset();
    m_device.Reset();
    if (m_com_initialized) CoUninitialize();
}

// ---------------------------------------------------------- device (once)

void AudioPlayer::init_system_audio() {
    // Qt may already have initialized COM as STA on this thread; that is fine.
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (hr == S_OK || hr == S_FALSE) {
        m_com_initialized = true;                // we must balance with CoUninitialize
    } else if (hr != RPC_E_CHANGED_MODE) {
        throw_if_failed(hr, "CoInitializeEx failed");
    }

    ComPtr<IMMDeviceEnumerator> enumerator;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          IID_PPV_ARGS(&enumerator));
    throw_if_failed(hr, "Failed to create device enumerator");

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);
    throw_if_failed(hr, "Failed to get default audio endpoint");

    hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                            reinterpret_cast<void**>(m_client.GetAddressOf()));
    throw_if_failed(hr, "Failed to activate IAudioClient");

    // Mix format is owned by COM -> free with CoTaskMemFree automatically.
    WAVEFORMATEX* raw = nullptr;
    hr = m_client->GetMixFormat(&raw);
    throw_if_failed(hr, "GetMixFormat failed");
    std::unique_ptr<WAVEFORMATEX, decltype(&CoTaskMemFree)> mix(raw, CoTaskMemFree);

    // Shared-mode mix formats are float32 in practice (tag is either
    // WAVE_FORMAT_IEEE_FLOAT or WAVE_FORMAT_EXTENSIBLE with the float subformat).
    const bool float_tag = mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
                           mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE;
    if (!float_tag || mix->wBitsPerSample != 32) {
        throw std::runtime_error("Unsupported device mix format (expected float32)");
    }
    m_dst_rate = mix->nSamplesPerSec;
    m_dst_channels = mix->nChannels;

    constexpr REFERENCE_TIME requested = 2'000'000;  // 200 ms in 100-ns units
    hr = m_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requested, 0, mix.get(), nullptr);
    throw_if_failed(hr, "Failed to initialize IAudioClient");

    hr = m_client->GetBufferSize(&m_buffer_frames);
    throw_if_failed(hr, "GetBufferSize failed");

    hr = m_client->GetService(__uuidof(IAudioRenderClient),
                              reinterpret_cast<void**>(m_render.GetAddressOf()));
    throw_if_failed(hr, "Failed to get IAudioRenderClient");

    hr = m_client->GetService(__uuidof(ISimpleAudioVolume),
                              reinterpret_cast<void**>(m_volume.GetAddressOf()));
    throw_if_failed(hr, "Failed to get ISimpleAudioVolume");

    // Wake up 4x per buffer.
    const auto buffer_ms = static_cast<long long>(m_buffer_frames) * 1000 / m_dst_rate;
    m_poll_interval = std::chrono::milliseconds(std::max<long long>(1, buffer_ms / 4));
}

// ------------------------------------------------------------ track loading

void AudioPlayer::load_track(size_t index) {
    const int total = m_play_list.total_tracks();
    if (index >= static_cast<size_t>(total)) {
        throw std::out_of_range("Track index out of range");
    }

    // Nothing may read m_pcm while we replace it.
    stop_worker_thread();
    m_client->Stop();
    m_client->Reset();                            // discard unplayed device buffer

    const Track& track = m_play_list.get_track(static_cast<int>(index));
    Wave wave(track.file_path);                   // decoded, then freed after conversion

    if (wave.audioFormat != WAVE_FORMAT_PCM || wave.bitsPerSample != 16) {
        throw std::runtime_error("Only 16-bit PCM WAV is supported for now");
    }

    m_pcm = convert_to_device(wave.m_audio_data, wave.sampleRate, wave.numChannels,
                              m_dst_rate, m_dst_channels);

    m_index = index;
    m_frame_pos = 0;
    m_finished = false;
    m_error = false;
}

// ---------------------------------------------------------------- transport

void AudioPlayer::play() {
    if (m_pcm.empty()) return;

    stop_worker_thread();                         // joins a finished/old worker safely

    const uint64_t total_frames = m_pcm.size() / m_dst_channels;
    if (m_frame_pos >= total_frames) {            // replay from the start
        m_client->Reset();
        m_frame_pos = 0;
    }
    m_finished = false;

    const HRESULT hr = m_client->Start();
    if (FAILED(hr)) {
        std::println(stderr, "Failed to start audio client (0x{:08X})", static_cast<unsigned>(hr));
        m_error = true;
        return;
    }
    m_playing = true;
    m_worker = std::thread(&AudioPlayer::play_audio, this);
}

void AudioPlayer::pause() {
    stop_worker_thread();
    if (m_client) m_client->Stop();               // buffered audio resumes on Start()
}

void AudioPlayer::next() {
    const int total = m_play_list.total_tracks();
    if (total == 0) return;
    load_track((m_index + 1) % total);
    play();
}

void AudioPlayer::previous() {
    const int total = m_play_list.total_tracks();
    if (total == 0) return;
    load_track((m_index + total - 1) % total);
    play();
}

void AudioPlayer::seek_player(int seconds) {
    if (m_pcm.empty()) return;

    const bool was_playing = m_playing;
    stop_worker_thread();
    m_client->Stop();
    m_client->Reset();

    const uint64_t total_frames = m_pcm.size() / m_dst_channels;
    const uint64_t target = static_cast<uint64_t>(std::max(0, seconds)) * m_dst_rate;
    m_frame_pos = std::min(target, total_frames);
    m_finished = false;

    if (was_playing) play();
}

void AudioPlayer::change_volume(int value) {
    m_volume_level = std::clamp(value, 0, 100);
    if (!m_volume) return;
    const float level = m_volume_level / 100.0f;
    const HRESULT hr = m_volume->SetMasterVolume(level, nullptr);
    if (FAILED(hr)) {
        std::println(stderr, "SetMasterVolume failed (0x{:08X})", static_cast<unsigned>(hr));
    }
}

int AudioPlayer::duration_seconds() const {
    if (m_dst_rate == 0 || m_dst_channels == 0) return 0;
    return static_cast<int>(m_pcm.size() / m_dst_channels / m_dst_rate);
}

int AudioPlayer::current_second() const {
    if (m_dst_rate == 0) return 0;
    return static_cast<int>(m_frame_pos / m_dst_rate);
}

// ------------------------------------------------------------ worker thread

void AudioPlayer::stop_worker_thread() {
    m_playing = false;
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void AudioPlayer::play_audio() {
    try {
        const uint64_t total_frames = m_pcm.size() / m_dst_channels;
        const size_t frame_bytes = m_dst_channels * sizeof(float);

        // Fill the device buffer until the whole track has been written.
        while (m_playing) {
            UINT32 padding = 0;
            HRESULT hr = m_client->GetCurrentPadding(&padding);
            throw_if_failed(hr, "GetCurrentPadding failed");

            const uint64_t pos = m_frame_pos;
            const uint64_t remaining = total_frames - pos;
            if (remaining == 0) break;

            const UINT32 available = m_buffer_frames - padding;
            const UINT32 n = static_cast<UINT32>(std::min<uint64_t>(available, remaining));

            if (n > 0) {
                BYTE* dst = nullptr;
                hr = m_render->GetBuffer(n, &dst);
                throw_if_failed(hr, "GetBuffer failed");

                std::memcpy(dst, m_pcm.data() + pos * m_dst_channels, n * frame_bytes);

                hr = m_render->ReleaseBuffer(n, 0);
                throw_if_failed(hr, "ReleaseBuffer failed");

                m_frame_pos = pos + n;
            }
            std::this_thread::sleep_for(m_poll_interval);
        }

        // Drain: let the already-written audio finish before reporting "done".
        bool reached_end = (m_frame_pos >= total_frames);
        while (reached_end && m_playing) {
            UINT32 padding = 0;
            if (FAILED(m_client->GetCurrentPadding(&padding)) || padding == 0) break;
            std::this_thread::sleep_for(m_poll_interval);
        }

        if (reached_end && m_playing) {
            m_client->Stop();
            m_playing = false;
            m_finished = true;                    // UI thread will call next()
        }
    } catch (const std::exception& e) {
        std::println(stderr, "Audio thread error: {}", e.what());
        m_error = true;
        m_playing = false;
    }
}