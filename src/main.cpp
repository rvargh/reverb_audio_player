#include "Common.hpp"
#include "MainWindow.hpp"
#include <QApplication>

int main(int argc, char *argv[])
{
    // fetching the mp3 name
    if (argc <= 1 || argv[1] == nullptr) {
        // TODO this need to change, player needs to open without file
        std::cerr << "Error: No MP3 file provided!\n";
        std::cerr << "Usage: " << argv[0] << " <path/to/music.mp3>\n";
        std::exit(EXIT_FAILURE);
    }
    try {
        std::cout << "I am running" << std::endl;

        QApplication app(argc, argv);

        //const std::string path = argv[1];

        AudioPlayer player;
        player.load(argv[1]);

        std::cout << argc << std::endl;
        std::cout << "hello from audio player";

        MainWindow window(&player);
        window.load_window();

        return app.exec();
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Caught an exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Caught an unknown exception!" << std::endl;
    }
}