cmake_minimum_required(VERSION 3.31)

set(CMAKE_C_COMPILER "C:/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/mingw64/bin/g++.exe")

project(reverb_audio_player)

# Safely bind GCC runtime without breaking std::println
add_link_options(-static)
link_libraries(stdc++exp)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.11.2/mingw_64/" ${CMAKE_PREFIX_PATH})
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Use a clean variable name for your executable target
set(PROJECT_TARGET reverb_audio_player)

add_executable(${PROJECT_TARGET}
        src/main.cpp
        src/Common.hpp
        src/MainWindow.cpp
        src/MainWindow.hpp
        src/AudioPlayer.hpp
        src/AudioPlayer.cpp
        src/Wave.hpp
        src/Wave.cpp
)

# Link Qt6 specifically to your new project target variable
target_link_libraries(${PROJECT_TARGET} PRIVATE Qt6::Widgets)
