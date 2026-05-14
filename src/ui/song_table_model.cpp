#include "song_table_model.h"

#include <QByteArray>
#include <QDataStream>
#include <QIODevice>
#include <QMimeData>
#include <QString>
#include <QStringList>

#include <utility>  // std::move, std::cmp_greater_equal
#include <vector>

namespace musicplayer {

namespace {

QString formatDuration(double seconds) {
    if (seconds <= 0.0)
        return QStringLiteral("--:--");
    const auto total = static_cast<int>(seconds);
    const int minutes = total / 60;
    const int secs = total % 60;
    return QString::asprintf("%d:%02d", minutes, secs);
}

}  // namespace

SongTableModel::SongTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int SongTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(songs_.size());
}

int SongTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 5;
}

QVariant SongTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || std::cmp_greater_equal(index.row(), songs_.size()))
        return {};
    const SongInfo& s = songs_[static_cast<std::size_t>(index.row())];
    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColumnIndex:
                return index.row() + 1;
            case ColumnTitle:
                return QString::fromStdString(s.title);
            case ColumnArtist:
                return QString::fromStdString(s.artist);
            case ColumnAlbum:
                return QString::fromStdString(s.album);
            case ColumnDuration:
                return formatDuration(s.duration);
            default:
                return {};
        }
    }
    if (role == Qt::TextAlignmentRole &&
        (index.column() == ColumnIndex || index.column() == ColumnDuration)) {
        return static_cast<int>(Qt::AlignCenter);
    }
    return {};
}

QVariant SongTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};
    switch (section) {
        case ColumnIndex:
            return QStringLiteral("#");
        case ColumnTitle:
            return QObject::tr("Title");
        case ColumnArtist:
            return QObject::tr("Artist");
        case ColumnAlbum:
            return QObject::tr("Album");
        case ColumnDuration:
            return QObject::tr("Duration");
        default:
            return {};
    }
}

void SongTableModel::setSongs(std::vector<SongInfo> songs) {
    beginResetModel();
    songs_ = std::move(songs);
    endResetModel();
}

int SongTableModel::songIdAt(int row) const {
    if (row < 0 || std::cmp_greater_equal(row, songs_.size()))
        return 0;
    return songs_[static_cast<std::size_t>(row)].id;
}

void SongTableModel::setReorderable(bool enable) {
    reorderable_ = enable;
}

namespace {
constexpr const char* kReorderMime = "application/x-musicplayer-song-row";
}  // namespace

Qt::ItemFlags SongTableModel::flags(const QModelIndex& index) const {
    const Qt::ItemFlags base = QAbstractTableModel::flags(index);
    if (!reorderable_)
        return base;
    if (index.isValid())
        return base | Qt::ItemIsDragEnabled;
    // Drops on the root index (gaps between rows) are how Qt asks the
    // model to receive a moved row; rows themselves should NOT accept drops
    // so the indicator lands between rows, not on top of them.
    return base | Qt::ItemIsDropEnabled;
}

Qt::DropActions SongTableModel::supportedDropActions() const {
    return Qt::MoveAction;
}

QStringList SongTableModel::mimeTypes() const {
    return {QString::fromLatin1(kReorderMime)};
}

QMimeData* SongTableModel::mimeData(const QModelIndexList& indexes) const {
    if (indexes.isEmpty())
        return nullptr;
    // Encode the source row index. Qt may pass us one index per column for
    // the same row — collapse to the first valid row.
    int sourceRow = -1;
    for (const auto& idx : indexes) {
        if (idx.isValid()) {
            sourceRow = idx.row();
            break;
        }
    }
    if (sourceRow < 0)
        return nullptr;
    auto* mime = new QMimeData;
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream << sourceRow;
    mime->setData(QString::fromLatin1(kReorderMime), bytes);
    return mime;
}

bool SongTableModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row,
                                  int /*column*/, const QModelIndex& parent) {
    if (action != Qt::MoveAction || !reorderable_)
        return false;
    if (!data->hasFormat(QString::fromLatin1(kReorderMime)))
        return false;

    QByteArray bytes = data->data(QString::fromLatin1(kReorderMime));
    QDataStream stream(&bytes, QIODevice::ReadOnly);
    int sourceRow = -1;
    stream >> sourceRow;
    if (sourceRow < 0 || std::cmp_greater_equal(sourceRow, songs_.size()))
        return false;

    // row == -1 (dropped onto root / past last item) → append.
    int destRow = row;
    if (destRow < 0)
        destRow = parent.isValid() ? parent.row() : rowCount();
    if (destRow == sourceRow || destRow == sourceRow + 1)
        return false;

    SongInfo moved = songs_[static_cast<std::size_t>(sourceRow)];
    songs_.erase(songs_.begin() + sourceRow);
    if (destRow > sourceRow)
        --destRow;  // the erase shifted indices left
    songs_.insert(songs_.begin() + destRow, std::move(moved));

    // beginResetModel / endResetModel is the simplest signal that's safe
    // here: a single moveRows call would need beginMoveRows/endMoveRows
    // with the awkward "destinationChild adjusted for source removal"
    // semantics. With a table this size, a reset is imperceptibly fast.
    beginResetModel();
    endResetModel();

    QList<int> ids;
    ids.reserve(static_cast<qsizetype>(songs_.size()));
    for (const auto& s : songs_)
        ids.append(s.id);
    emit songsReordered(ids);

    // Returning true tells Qt the drop succeeded; Qt's InternalMove will
    // follow up with removeRows(sourceRow, 1) to "clean up" the source
    // row. We've already performed the move ourselves, so the override of
    // removeRows below is a no-op.
    return true;
}

bool SongTableModel::removeRows(int /*row*/, int /*count*/, const QModelIndex& /*parent*/) {
    // See dropMimeData: we never want Qt's auto-cleanup to mutate songs_,
    // and no other call site of removeRows exists. Returning true keeps
    // the view's drag-drop state machine happy.
    return true;
}

}  // namespace musicplayer
