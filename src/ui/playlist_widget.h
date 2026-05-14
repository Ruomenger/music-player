#pragma once

#include <QList>
#include <QWidget>

#include <vector>

#include "song_info.h"

class QModelIndex;
class QPoint;
class QTableView;

namespace musicplayer {

class SongTableModel;

// QTableView host for the library / playlist song list. Forwards user intent
// upward as signals (songActivated, removeSongRequested, toggleFavoriteRequested,
// songsReorderedSignal); the host owns the data model and decides which actions
// make sense for the current source. Drag-drop reorder is enabled per call to
// setReorderable() — only inside user playlists.
class PlaylistWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistWidget(QWidget* parent = nullptr);

    void setSongs(std::vector<SongInfo> songs);
    void setReorderable(bool enable);
    void setRemovable(bool enable);  // affects context menu: show "Remove" or not

    [[nodiscard]] SongTableModel* model() const { return model_; }

signals:
    void songActivated(int songId);
    void removeSongRequested(int songId);
    void toggleFavoriteRequested(int songId);
    void songsReorderedSignal(const QList<int>& orderedSongIds);

private slots:
    void onActivated(const QModelIndex& index);
    void onContextMenu(const QPoint& pos);

private:
    SongTableModel* model_;
    QTableView* view_;
    bool removable_ = false;
};

}  // namespace musicplayer
