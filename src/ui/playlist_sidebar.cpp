#include "playlist_sidebar.h"

#include <QAction>
#include <QFont>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

#include <algorithm>

namespace musicplayer {

namespace {

// Custom roles attached to each row's QListWidgetItem so the activation
// handler can recover the source kind + playlist id without parsing labels.
constexpr int kSourceRole = Qt::UserRole + 1;
constexpr int kPlaylistIdRole = Qt::UserRole + 2;

QListWidgetItem* makeHeader(const QString& text) {
    auto* item = new QListWidgetItem(text);
    QFont f = item->font();
    f.setBold(true);
    item->setFont(f);
    item->setFlags(Qt::NoItemFlags);  // not selectable, not clickable
    return item;
}

QListWidgetItem* makeSourceRow(const QString& text, LibrarySource source, int playlistId = 0) {
    auto* item = new QListWidgetItem(text);
    item->setData(kSourceRole, static_cast<int>(source));
    item->setData(kPlaylistIdRole, playlistId);
    return item;
}

}  // namespace

PlaylistSidebar::PlaylistSidebar(QWidget* parent)
    : QWidget(parent)
    , list_(new QListWidget(this))
    , newBtn_(new QPushButton(tr("+ New Playlist"), this)) {
    list_->setContextMenuPolicy(Qt::CustomContextMenu);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(newBtn_);
    layout->addWidget(list_, /*stretch=*/1);

    connect(newBtn_, &QPushButton::clicked, this, &PlaylistSidebar::newPlaylistRequested);
    connect(list_, &QListWidget::itemClicked, this, &PlaylistSidebar::onItemActivated);
    connect(list_, &QListWidget::customContextMenuRequested, this, &PlaylistSidebar::onContextMenu);

    rebuild();
}

void PlaylistSidebar::setPlaylists(const std::vector<Playlist>& playlists) {
    playlists_ = playlists;
    rebuild();
}

void PlaylistSidebar::selectPlaylist(int playlistId) {
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        const auto source = static_cast<LibrarySource>(item->data(kSourceRole).toInt());
        if (source == LibrarySource::UserPlaylist &&
            item->data(kPlaylistIdRole).toInt() == playlistId) {
            list_->setCurrentItem(item);
            emit sourceSelected(source, playlistId);
            return;
        }
    }
}

void PlaylistSidebar::selectAllSongs() {
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        if (static_cast<LibrarySource>(item->data(kSourceRole).toInt()) ==
            LibrarySource::AllSongs) {
            list_->setCurrentItem(item);
            return;
        }
    }
}

void PlaylistSidebar::onItemActivated(QListWidgetItem* item) {
    if (item == nullptr || !(item->flags() & Qt::ItemIsSelectable))
        return;
    const auto source = static_cast<LibrarySource>(item->data(kSourceRole).toInt());
    const int playlistId = item->data(kPlaylistIdRole).toInt();
    emit sourceSelected(source, playlistId);
}

void PlaylistSidebar::onContextMenu(const QPoint& pos) {
    auto* item = list_->itemAt(pos);
    if (!item)
        return;
    const auto source = static_cast<LibrarySource>(item->data(kSourceRole).toInt());
    if (source != LibrarySource::UserPlaylist)
        return;  // Sources / system playlists are read-only — no menu
    const int playlistId = item->data(kPlaylistIdRole).toInt();

    // System playlists ("我喜欢") still expose UserPlaylist source kind but
    // we skip them by looking up the original Playlist record.
    const auto found = std::ranges::find_if(
        playlists_, [playlistId](const Playlist& p) { return p.id == playlistId; });
    if (found == playlists_.end() || found->isSystem)
        return;

    QMenu menu(this);
    auto* renameAction = menu.addAction(tr("Rename…"));
    auto* deleteAction = menu.addAction(tr("Delete"));
    QAction* picked = menu.exec(list_->mapToGlobal(pos));
    if (picked == renameAction) {
        emit renamePlaylistRequested(playlistId);
    } else if (picked == deleteAction) {
        emit deletePlaylistRequested(playlistId);
    }
}

void PlaylistSidebar::rebuild() {
    list_->clear();

    list_->addItem(makeHeader(tr("Sources")));
    list_->addItem(makeSourceRow(tr("All Songs"), LibrarySource::AllSongs));
    list_->addItem(makeSourceRow(tr("Recently Played"), LibrarySource::RecentlyPlayed));

    list_->addItem(makeHeader(tr("Playlists")));
    for (const auto& p : playlists_) {
        const QString prefix = p.isSystem ? QStringLiteral("♥ ") : QString();
        list_->addItem(makeSourceRow(prefix + QString::fromStdString(p.name),
                                     LibrarySource::UserPlaylist, p.id));
    }
}

}  // namespace musicplayer
