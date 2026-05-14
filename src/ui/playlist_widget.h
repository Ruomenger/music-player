#pragma once

#include <QWidget>

#include <vector>

#include "song_info.h"

class QModelIndex;
class QTableView;

namespace musicplayer {

class SongTableModel;

// Thin QTableView wrapper bound to a SongTableModel. Emits songActivated()
// with the underlying song id when the user double-clicks (or presses Enter
// on) a row, so MainWindow can hand it to PlayerController without the UI
// touching the player directly.
class PlaylistWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistWidget(QWidget* parent = nullptr);

    void setSongs(std::vector<SongInfo> songs);
    [[nodiscard]] SongTableModel* model() const { return model_; }

signals:
    void songActivated(int songId);

private slots:
    void onActivated(const QModelIndex& index);

private:
    SongTableModel* model_;
    QTableView* view_;
};

}  // namespace musicplayer
