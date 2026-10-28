#include "MainWindow.hpp"
#include  "AudioPlayer.hpp"

MainWindow::MainWindow(AudioPlayer* player) : m_audio_player(player) {
}

QString MainWindow::format_time(int total_seconds) {
    int minutes = total_seconds / 60;
    int seconds = total_seconds % 60;

    return QString("%1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::load_window() {
    QMainWindow* main_window = this;
    main_window->setWindowTitle("Audio Player");
    main_window->resize(400, 130);   // smaller window

    main_window->setStyleSheet(R"(
        QMainWindow, QWidget {
            background-color: #121212;
            color: #ffffff;
            font-family: "Segoe UI", "SF Pro Display", sans-serif;
        }

        QLabel#titleLabel {
            font-size: 13px;
            font-weight: 600;
            color: #ffffff;
        }

        QLabel#artistLabel {
            font-size: 11px;
            color: #b3b3b3;
        }

        QLabel#timeLabel {
            font-size: 10px;
            color: #b3b3b3;
            min-width: 30px;
        }

        QSlider::groove:horizontal {
            height: 4px;
            background: #404040;
            border-radius: 2px;
        }

        QSlider::handle:horizontal {
            background: #ffffff;
            width: 10px;
            height: 10px;
            margin: -3px 0;
            border-radius: 5px;
        }

        QSlider::sub-page:horizontal {
            background: #1db954;
            border-radius: 2px;
        }

        QPushButton {
            background-color: transparent;
            border: none;
            color: #ffffff;
            font-size: 13px;
            padding: 4px 8px;
            border-radius: 12px;
        }

        QPushButton:hover {
            background-color: #282828;
        }

        QPushButton:pressed {
            background-color: #1db954;
        }

        QPushButton#playButton {
            font-size: 15px;
            padding: 5px 9px;
        }
    )");

    QWidget* central_widget = new QWidget();
    QVBoxLayout* main_layout = new QVBoxLayout(central_widget);
    main_layout->setContentsMargins(12, 10, 12, 10);
    main_layout->setSpacing(6);                     // reduced vertical gap

    // ---------- Top: Album Art + Title / Artist ----------
    QHBoxLayout* top_layout = new QHBoxLayout();
    top_layout->setSpacing(10);

    QLabel* album_art = new QLabel();
    album_art->setFixedSize(48, 48);
    album_art->setStyleSheet("background-color: #282828; border-radius: 4px;");

    QVBoxLayout* info_layout = new QVBoxLayout();
    info_layout->setSpacing(0);
    info_layout->setContentsMargins(0, 1, 0, 0);

    QLabel* title = new QLabel("Song Title");
    title->setObjectName("titleLabel");

    QLabel* artist = new QLabel("Artist : Binsy");
    artist->setObjectName("artistLabel");

    info_layout->addWidget(title);
    info_layout->addWidget(artist);
    info_layout->addStretch();

    top_layout->addWidget(album_art);
    top_layout->addLayout(info_layout, 1);

    // ---------- Progress ----------
    QHBoxLayout* progress_layout = new QHBoxLayout();
    progress_layout->setSpacing(5);

    QLabel* current_time = new QLabel("0:00");
    current_time->setObjectName("timeLabel");

    QSlider* progress = new QSlider(Qt::Horizontal);
    progress->setRange(0, m_audio_player->duration_seconds());
    progress->setValue(0);

    QLabel* remaining_time = new QLabel("-0:00");
    remaining_time->setObjectName("timeLabel");

    progress_layout->addWidget(current_time);
    progress_layout->addWidget(progress, 1);
    progress_layout->addWidget(remaining_time);

    // Timer
    QTimer* progress_timer = new QTimer(main_window);
    QObject::connect(progress_timer, &QTimer::timeout, this,
        [this, progress, current_time, remaining_time]() {
            int current_sec = m_audio_player->current_second();
            progress->setValue(current_sec);
            current_time->setText(format_time(current_sec));

            int remaining = std::max(0, m_audio_player->duration_seconds() - current_sec);
            remaining_time->setText("-" + format_time(remaining));
        });
    progress_timer->start(1000);

    // Seek
    QObject::connect(progress, &QSlider::sliderReleased, this, [this, progress]() {
        m_audio_player->seek_player(progress->value());
    });

    // ---------- Controls + Volume (much tighter) ----------
    QHBoxLayout* controls_layout = new QHBoxLayout();
    controls_layout->setSpacing(2);                 // very tight spacing

    QPushButton* previous = new QPushButton("⏮");
    QPushButton* play = new QPushButton("▶");
    play->setObjectName("playButton");
    QPushButton* next = new QPushButton("⏭");

    QObject::connect(previous, &QPushButton::clicked, this, [this]() {
        m_audio_player->previous();
    });

    QObject::connect(play, &QPushButton::clicked, this, [this, play]() {
        if (m_audio_player->is_playing()) {
            m_audio_player->pause();
            play->setText("▶");
        } else {
            m_audio_player->play();
            play->setText("⏸");
        }
    });

    QObject::connect(next, &QPushButton::clicked, this, [this]() {
        m_audio_player->next();
    });

    QSlider* volume = new QSlider(Qt::Horizontal);
    volume->setRange(0, 100);
    volume->setValue(15);
    volume->setFixedWidth(60);
    m_audio_player->change_volume(volume->value());

    QObject::connect(volume, &QSlider::sliderMoved, this, [this](int value) {
        m_audio_player->change_volume(value);
    });

    controls_layout->addStretch();
    controls_layout->addWidget(previous);
    controls_layout->addWidget(play);
    controls_layout->addWidget(next);
    controls_layout->addSpacing(10);                // small gap before volume
    controls_layout->addWidget(volume);
    controls_layout->addStretch();

    // ---------- Assemble ----------
    main_layout->addLayout(top_layout);
    main_layout->addLayout(progress_layout);
    main_layout->addLayout(controls_layout);

    main_window->setCentralWidget(central_widget);
    main_window->show();
}