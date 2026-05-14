#include "playlist_widget.h"

#include <QHeaderView>
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
    // The index column ("#") and the Duration column are narrow; let
    // Title / Artist / Album breathe with the leftover space.
    view_->horizontalHeader()->resizeSection(SongTableModel::ColumnIndex, 40);
    view_->horizontalHeader()->resizeSection(SongTableModel::ColumnDuration, 70);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_);

    connect(view_, &QTableView::doubleClicked, this, &PlaylistWidget::onActivated);
    connect(view_, &QTableView::activated, this, &PlaylistWidget::onActivated);
}

void PlaylistWidget::setSongs(std::vector<SongInfo> songs) {
    model_->setSongs(std::move(songs));
}

void PlaylistWidget::onActivated(const QModelIndex& index) {
    const int id = model_->songIdAt(index.row());
    if (id > 0)
        emit songActivated(id);
}

}  // namespace musicplayer
