#include  "Playlist.hpp"

void PlayList::add_tracks(Track track) {
    m_tracks.push_back(std::move(track));
}

Track& PlayList::get_track(const int key) {
    return m_tracks.at(key);
}

const Track& PlayList::get_track(const int key) const {
    return m_tracks.at(key);
}

int PlayList::total_tracks() const {
    return m_tracks.size();
}