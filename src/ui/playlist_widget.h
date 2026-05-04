#pragma once

#include <QWidget>

namespace musicplayer {

class PlaylistWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistWidget(QWidget* parent = nullptr);
};

} // namespace musicplayer
