#include "Common.hpp"

class Wave {

    public:
        char chunkID[4] = {0};
        uint32_t chunkSize = 0;
        char format[4] = {0};
        char subchunk1ID[4] = {0};
        uint32_t subchunk1Size = 0;
        uint16_t audioFormat = 0; // 1 => PCM
        uint16_t numChannels = 0;
        uint32_t sampleRate = 0;
        uint32_t byteRate = 0;
        uint16_t blockAlign = 0;
        uint16_t bitsPerSample = 0;
        char subchunk2ID[5] = {0};
        uint32_t subchunk2Size = 0;
        std::vector<short> m_audio_data = {};
        std::vector<float> m_float_data = {};
        std::variant<std::vector<float>,std::vector<short>> multi_format_buffer;

    public:
        explicit Wave(std::filesystem::path path);
        char& operator[](int index);
        void wave_parser();
        void erase_audio(uint16_t size, bool is_float);
        void convert_to_float();

    private:
        std::filesystem::path m_file_path;
};