#pragma once

#include "Common.hpp"

class PlayList {

    public:
        void add_tracks(Track track);

        Track& get_track(int key);
        const Track& get_track(int key) const;
        size_t total_tracks() const;

    private:
        std::vector<Track> m_tracks;
};
