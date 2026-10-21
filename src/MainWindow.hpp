#pragma once

#include "Common.hpp"
#include "AudioPlayer.hpp"

class MainWindow {
    public:
        explicit MainWindow(AudioPlayer* player);
        void load_window();


    private:
        AudioPlayer* m_audio_player;
};
