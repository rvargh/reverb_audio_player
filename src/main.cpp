#include "Common.hpp"
#include "MainWindow.hpp"
#include "Playlist.hpp"
#include <QApplication>

int main(int argc, char* argv[]) {
    try {
        std::cout << "I am running" << std::endl;
        QApplication app(argc, argv);
        PlayList playlist;

        for (int i = 1; i < argc; ++i) {
            // start from 1 (skip program name)
            if (argv[i] == nullptr) continue;

            std::filesystem::path p = argv[i];
            if (p.empty()) continue;

            TagLib::FileRef f(argv[i]);
            if (!f.isNull() && f.tag()) {
                Track track{};
                const TagLib::Tag* tag = f.tag();
                track.title = tag->title().toCString(true); // true = UTF-8
                track.album = tag->album().toCString(true);
                track.artist = tag->artist().toCString(true);
                track.file_path = argv[i];
                playlist.add_tracks(std::move(track));
            }
        }
        //const std::string path = argv[1];
        AudioPlayer player{playlist};
        //player.load();

        std::cout << argc << std::endl;
        std::cout << "hello from audio player";

        MainWindow window(&player);
        window.load_window();

        return app.exec();
    } catch (const std::runtime_error &e) {
        std::cerr << "Caught an exception: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        // Catches any other standard C++ exceptions (e.g., bad_alloc, runtime_error)
        std::cerr << "Standard Exception Caught: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Caught an unknown exception!" << std::endl;
    }
}
