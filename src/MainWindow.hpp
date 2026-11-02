#pragma once

#include "Common.hpp"
#include "AudioPlayer.hpp"

class MainWindow : public QMainWindow
{
    public:
        explicit MainWindow(AudioPlayer* player);
        void load_window();
        void update_now_playing();

        static QString format_time(int total_seconds);

    private:
        AudioPlayer* m_audio_player;
        QLabel* m_title_label  = nullptr;
        QLabel* m_artist_label = nullptr;

        int m_last_index = -1;
};
