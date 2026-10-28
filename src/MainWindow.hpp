#pragma once

#include "Common.hpp"
#include "AudioPlayer.hpp"

class MainWindow : public QMainWindow
{
    public:
        explicit MainWindow(AudioPlayer* player);
        void load_window();

        static QString format_time(int total_seconds);

    private:
        AudioPlayer* m_audio_player;
};
