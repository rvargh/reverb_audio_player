#pragma once

// common c++ libs
#include <iostream>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <vector>
#include <thread>
#include <chrono>
#include <variant>
#include <optional>
#include <atomic>
#include <print>
#include <algorithm>

// QT libs
#include <QMainWindow>
#include <QMenuBar>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QFileInfo>

// taglib
#include <taglib/fileref.h>
#include <taglib/tag.h>

//WASAPI

// MACROS
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

struct WaveStruct{
    char riff_struct[5];
};

struct Track {
    std::filesystem::path file_path;
    std::string title;
    std::string artist;
    std::string album;
};