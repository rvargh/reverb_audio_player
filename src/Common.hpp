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

// QT libs
#include <QMainWindow>
#include <QMenuBar>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>

//WASAPI

// MACROS
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

struct WaveStruct{
    char riff_struct[5];
};