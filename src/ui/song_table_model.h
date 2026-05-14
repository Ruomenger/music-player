#pragma once

#include <QAbstractTableModel>

#include <vector>

#include "song_info.h"

namespace musicplayer {

// Read-only table model backing PlaylistWidget. Holds a copy of a SongInfo
// vector and renders five columns: row index, title, artist, album, duration
// (formatted m:ss). Rebuilds via setSongs() — granular insert/remove is left
// for Phase 7 when drag-and-drop reorder lands.
class SongTableModel : public QAbstractTableModel {
    Q_OBJECT
public:
    // Qt's column() API uses int, so int is the right base type here. Reducing
    // to uint8_t would force casts at every comparison and gain nothing.
    // Unscoped — these constants are compared directly against int column
    // indices throughout the table model and tests; wrapping each use in
    // static_cast<int> would obscure the call sites.
    // NOLINTNEXTLINE(performance-enum-size, cppcoreguidelines-use-enum-class)
    enum Column : int { ColumnIndex = 0, ColumnTitle, ColumnArtist, ColumnAlbum, ColumnDuration };

    explicit SongTableModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;

    void setSongs(std::vector<SongInfo> songs);
    [[nodiscard]] const std::vector<SongInfo>& songs() const { return songs_; }

    // Returns the song id at the given row, or 0 if row is out of range. UI
    // emits this when the user double-clicks; PlayerController takes the id.
    [[nodiscard]] int songIdAt(int row) const;

private:
    std::vector<SongInfo> songs_;
};

}  // namespace musicplayer
