
#include "MainWindow.hpp"
#include  "AudioPlayer.hpp"

MainWindow::MainWindow(AudioPlayer* player) : m_audio_player(player) {}

void MainWindow::load_window() {
    QMainWindow* main_window = new QMainWindow();
    main_window->setWindowTitle("Audio Player");
    main_window->resize(500, 150);

    QWidget* central_widget = new QWidget();

    QVBoxLayout* player_layout = new QVBoxLayout();

    QLabel* albumArt = new QLabel("Album Art : ");

    QLabel* title = new QLabel("Song Title : ");
    QLabel* artist = new QLabel("Artist : Binsy");
    QLabel* volume_label = new QLabel("Volume : ");

    QSlider* progress = new QSlider(Qt::Horizontal);
    progress->setRange(0, m_audio_player->duration_seconds());
    progress->setValue(0);
    QObject::connect(progress, &QSlider::sliderReleased, [this, progress]() {
        int targetSecond = progress->value();
        // Call your audio engine's seek function
        // m_audio_player->seek_to_second(targetSecond);
    });

    QSlider* volume = new QSlider(Qt::Horizontal);
    volume->setRange(0, 100);
    volume->setValue(15);
    QObject::connect(volume, &QSlider::sliderMoved, [this](int const value) {
        m_audio_player->set_volume(value);
    });

    QHBoxLayout* controls = new QHBoxLayout();
    QPushButton* previous = new QPushButton("◀◀");

    QObject::connect(previous, &QPushButton::clicked, [this]() {
        m_audio_player->previous();
    });
    QPushButton* play = new QPushButton("▶");

    QObject::connect(play, &QPushButton::clicked, [this,play]() {
        if (m_audio_player->is_playing()) {
            m_audio_player->pause();
            std::println("I am pausing the audio");
            play->setText("▶"); // Show the play symbol so they can resume
        } else {
            m_audio_player->play();
            play->setText("⏸"); // Show the pause symbol while it runs
        }
    });

    QPushButton* next = new QPushButton("►►");
    QObject::connect(next, &QPushButton::clicked, [this]() {
        m_audio_player->next();
    });

    QHBoxLayout* seek_volume_controls = new QHBoxLayout();

    volume->setFixedWidth(100);
    seek_volume_controls->addWidget(progress);
    seek_volume_controls->addWidget(volume_label);
    seek_volume_controls->addWidget(volume);

    controls->addWidget(previous);
    controls->addWidget(play);
    controls->addWidget(next);

    player_layout->addWidget(albumArt);
    player_layout->addWidget(title);
    player_layout->addWidget(artist);
    player_layout->addLayout(controls);
    player_layout->addLayout(seek_volume_controls);

    player_layout->addStretch();
    central_widget->setLayout(player_layout);

    main_window->setCentralWidget(central_widget);

    main_window->show();
}
