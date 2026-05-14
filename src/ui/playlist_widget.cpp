#include "playlist_widget.h"

#include <QAbstractItemView>
#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QModelIndex>
#include <QTableView>
#include <QVBoxLayout>

#include <utility>

#include "song_table_model.h"

namespace musicplayer {

PlaylistWidget::PlaylistWidget(QWidget* parent)
    : QWidget(parent), model_(new SongTableModel(this)), view_(new QTableView(this)) {
    view_->setModel(model_);
    view_->setSelectionBehavior(QAbstractItemView::SelectRows);
    view_->setSelectionMode(QAbstractItemView::SingleSelection);
    view_->setAlternatingRowColors(true);
    view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view_->verticalHeader()->setVisible(false);
    view_->horizontalHeader()->setStretchLastSection(true);
    view_->horizontalHeader()->resizeSection(SongTableModel::ColumnIndex, 40);
    view_->horizontalHeader()->resizeSection(SongTableModel::ColumnDuration, 70);

    // Drag-drop wiring (only takes effect when setReorderable(true) is
    // called; the model's flags() reflects the same toggle).
    view_->setDragEnabled(true);
    view_->setAcceptDrops(true);
    view_->setDropIndicatorShown(true);
    view_->setDragDropMode(QAbstractItemView::InternalMove);
    view_->setDragDropOverwriteMode(false);
    view_->setDefaultDropAction(Qt::MoveAction);

    view_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_);

    connect(view_, &QTableView::doubleClicked, this, &PlaylistWidget::onActivated);
    connect(view_, &QTableView::activated, this, &PlaylistWidget::onActivated);
    connect(view_, &QTableView::customContextMenuRequested, this, &PlaylistWidget::onContextMenu);
    connect(model_, &SongTableModel::songsReordered, this, &PlaylistWidget::songsReorderedSignal);
}

void PlaylistWidget::setSongs(std::vector<SongInfo> songs) {
    model_->setSongs(std::move(songs));
}

void PlaylistWidget::setReorderable(bool enable) {
    model_->setReorderable(enable);
    // The view's drag/accept toggles aren't strictly necessary (the model's
    // flags() controls behavior), but disabling them gives Qt cheaper paint
    // / hit tests for read-only views.
    view_->setDragEnabled(enable);
    view_->setAcceptDrops(enable);
}

void PlaylistWidget::setRemovable(bool enable) {
    removable_ = enable;
}

void PlaylistWidget::onActivated(const QModelIndex& index) {
    const int id = model_->songIdAt(index.row());
    if (id > 0)
        emit songActivated(id);
}

void PlaylistWidget::onContextMenu(const QPoint& pos) {
    const QModelIndex index = view_->indexAt(pos);
    if (!index.isValid())
        return;
    const int songId = model_->songIdAt(index.row());
    if (songId <= 0)
        return;

    QMenu menu(this);
    auto* playAction = menu.addAction(tr("Play"));
    auto* favoriteAction = menu.addAction(tr("Toggle favorite"));
    QAction* removeAction = nullptr;
    if (removable_) {
        menu.addSeparator();
        removeAction = menu.addAction(tr("Remove from playlist"));
    }
    QAction* picked = menu.exec(view_->viewport()->mapToGlobal(pos));
    if (picked == playAction) {
        emit songActivated(songId);
    } else if (picked == favoriteAction) {
        emit toggleFavoriteRequested(songId);
    } else if (removeAction != nullptr && picked == removeAction) {
        emit removeSongRequested(songId);
    }
}

}  // namespace musicplayer
