#pragma once

#include <QAbstractTableModel>
#include <QList>

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

    // ── Drag-drop reorder ───────────────────────────────────────────────
    // Switching reordering on opts the model into Qt's InternalMove flow:
    // flags() exposes ItemIsDragEnabled, mimeData / dropMimeData round-trip
    // the source row, and a successful drop emits songsReordered with the
    // new id ordering. The PlaylistWidget host flips this on when the user
    // navigates into a user playlist source and off again for read-only
    // views like "All Songs".
    void setReorderable(bool enable);
    [[nodiscard]] bool isReorderable() const { return reorderable_; }

    // QAbstractItemModel overrides for drag-drop:
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column,
                      const QModelIndex& parent) override;
    // Qt's InternalMove framework calls removeRows() on the source after a
    // successful dropMimeData to "clean up" the dragged row — but
    // dropMimeData already moved the song to its new index, so the cleanup
    // would corrupt the list. Overriding to a true no-op keeps the model
    // honest.
    bool removeRows(int row, int count, const QModelIndex& parent = {}) override;

signals:
    // Emitted after dropMimeData has finished applying a successful reorder.
    // Payload: the song ids in their new order. PlaylistWidget forwards this
    // up to MainWindow which calls PlaylistManager::reorderSongs. Using
    // QList<int> here so QSignalSpy / QVariant marshal the payload without
    // any Q_DECLARE_METATYPE dance.
    void songsReordered(const QList<int>& orderedSongIds);

private:
    std::vector<SongInfo> songs_;
    bool reorderable_ = false;
};

}  // namespace musicplayer
