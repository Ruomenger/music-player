#include "song_table_model.h"

#include <QString>

#include <utility>  // std::move, std::cmp_greater_equal

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

}  // namespace musicplayer
