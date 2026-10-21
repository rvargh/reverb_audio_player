cmake_minimum_required(VERSION 3.31)
project(audio_player)

# Force your executable to statically bind GCC internal structures
add_link_options(-static)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.2/mingw_64/" ${CMAKE_PREFIX_PATH})

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(audio_player
        src/main.cpp
        src/Common.hpp
        src/MainWindow.cpp
        src/MainWindow.hpp
        src/AudioPlayer.hpp
        src/AudioPlayer.cpp
        src/Wave.hpp
        src/Wave.cpp)
target_link_libraries(audio_player PRIVATE Qt6::Widgets)