#include "Wave.hpp"

#include <utility>

Wave::Wave(std::filesystem::path path) : m_file_path(std::move(path)) {
    wave_parser();
}

char& Wave::operator[](const int index) {
    return chunkID[index];
}

void Wave::wave_parser() {

    std::ifstream file(m_file_path, std::ios::binary);

    // 1. Read RIFF header
    file.read(chunkID, 4);
    file.read(reinterpret_cast<char*>(&chunkSize), 4);
    file.read(format, 4);

    // 2. Read fmt subchunk
    file.read(subchunk1ID, 4);
    file.read(reinterpret_cast<char*>(&subchunk1Size), 4);
    file.read(reinterpret_cast<char*>(&audioFormat), 2);
    file.read(reinterpret_cast<char*>(&numChannels), 2);
    file.read(reinterpret_cast<char*>(&sampleRate), 4);
    file.read(reinterpret_cast<char*>(&byteRate), 4);
    file.read(reinterpret_cast<char*>(&blockAlign), 2);
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    // 3. Read data subchunk header
    file.read(subchunk2ID, 4);
    file.read(reinterpret_cast<char*>(&subchunk2Size), 4);
    // here skipping rest of chunks to extract data
    if (std::string_view(subchunk2ID, 4) != "data") {
        file.seekg(subchunk2Size,std::ifstream::cur);
        file.read(subchunk2ID, 4);
        file.read(reinterpret_cast<char*>(&subchunk2Size), 4);
    }

    m_audio_data.resize(subchunk2Size / sizeof(int16_t));

    file.read(reinterpret_cast<std::istream::char_type*>(m_audio_data.data()),subchunk2Size);
  //  multi_format_buffer = std::move(m_audio_data);

    std::cout << "TLLG : " << file.tellg() << std::endl;
    std::cout << "COUNT" << file.gcount() << "\n";
}

void Wave::erase_audio(const uint16_t size, const bool is_float) {

    if (is_float) {
        m_float_data.erase(
            m_float_data.begin(),
            m_float_data.begin() + size
        );
    }
    else {
        m_audio_data.erase(
            m_audio_data.begin(),
            m_audio_data.begin() + size
        );
    }
}

void Wave::convert_to_float() {
    m_float_data.reserve(m_audio_data.size());
    for (const int16_t sample : m_audio_data) {
        m_float_data.push_back(static_cast<float>(sample) / 32768.0f);
    }
    //multi_format_buffer = std::move(m_float_data);
}
