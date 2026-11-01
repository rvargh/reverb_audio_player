#include "Common.hpp"
#include "MainWindow.hpp"
#include "Playlist.hpp"
#include <QApplication>

int main(int argc, char* argv[]) {
    try {
        std::cout << "I am running" << std::endl;
        QApplication app(argc, argv);
        PlayList playlist;

        if (argc <= 1 || argv[1] != nullptr) {
            if (TagLib::FileRef file_ref(argv[1]); !file_ref.isNull() && file_ref.tag()) {
                Track track = {};
                const TagLib::Tag* tag = file_ref.tag();
                track.title = tag->title().toCString();
                track.album = tag->album().toCString();
                track.artist = tag->artist().toCString();
                track.file_path = argv[1];
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
