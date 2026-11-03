#include  "Playlist.hpp"

void PlayList::add_tracks(Track track) {
    m_tracks.push_back(std::move(track));
}

Track& PlayList::get_track(const int key) {
    return m_tracks.at(key);
}

const Track& PlayList::get_track(int key) const {

    if (static_cast<std::size_t>(key) >= m_tracks.size()) {
        key = 0;
        std::cerr << "Requested out of range music \n";
    }
    return m_tracks[key];
}

size_t PlayList::total_tracks() const {
    return m_tracks.size();
}