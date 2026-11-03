#include "Wave.hpp"


#include <utility>

Wave::Wave(std::filesystem::path path) : m_file_path(std::move(path)) {
    wave_parser();
}

char &Wave::operator[](const int index) {
    return chunkID[index];
}

void Wave::wave_parser() {
    std::ifstream file(m_file_path, std::ios::binary);
    // 1. Read RIFF header
    file.read(chunkID, 4);
    file.read(reinterpret_cast<char *>(&chunkSize), 4);
    file.read(format, 4);

    // 2. Read fmt subchunk
    file.read(subchunk1ID, 4);
    file.read(reinterpret_cast<char *>(&subchunk1Size), 4);
    file.read(reinterpret_cast<char *>(&audioFormat), 2);
    file.read(reinterpret_cast<char *>(&numChannels), 2);
    file.read(reinterpret_cast<char *>(&sampleRate), 4);
    file.read(reinterpret_cast<char *>(&byteRate), 4);
    file.read(reinterpret_cast<char *>(&blockAlign), 2);
    file.read(reinterpret_cast<char *>(&bitsPerSample), 2);

    if (audioFormat == WAVE_FORMAT_EXTENSIBLE) {
        file.read(reinterpret_cast<char *>(&cbSize), 2);
        file.read(reinterpret_cast<char *>(&validBitsPerSample), 2);
        file.read(reinterpret_cast<char *>(&channelMask), 4);
        file.read(reinterpret_cast<char *>(&sub_format), sizeof(sub_format));
    }
    // 3. Read data subchunk header
    while (file.read(subchunk2ID, 4)) {
        file.read(reinterpret_cast<char *>(&subchunk2Size), 4);
        std::println("I am printing value inside the wave file : {}", subchunk2ID);

        if (std::string_view(subchunk2ID, 4) == "data") {
            break;
        }
        file.seekg(subchunk2Size, std::ifstream::cur);
    }
    //
    // if (IsEqualGUID(sub_format, KSDATAFORMAT_SUBTYPE_PCM)) {
    //     std::cout << "Integer PCM\n";
    // } else if (IsEqualGUID(sub_format, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
    //     std::cout << "IEEE Float\n";
    // } else {
    //     throw std::runtime_error("Unsupported audio encoding");
    // }


    // file.read(reinterpret_cast<char *>(&subchunk2Size), 4);

    // here skipping rest of chunks to extract data
    // if (std::string_view(subchunk2ID, 4) != "data") {
    //     file.seekg(subchunk2Size, std::ifstream::cur);
    //     file.read(subchunk2ID, 4);
    //
    // }

    if (bitsPerSample == 32) {
        m_32_int_data.resize(subchunk2Size / sizeof(int32_t));
        file.read(reinterpret_cast<std::istream::char_type *>(m_32_int_data.data()), subchunk2Size);
    }

    //  multi_format_buffer = std::move(m_audio_data);

}

void Wave::convert_to_float() {
    m_float_data.reserve(m_32_int_data.size());
    for (const int32_t sample: m_32_int_data) {
        m_float_data.push_back(static_cast<float>(sample) / 2147483648.0f);
    }
    //multi_format_buffer = std::move(m_float_data);
}
