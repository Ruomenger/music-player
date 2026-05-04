#pragma once

#include <QWidget>

namespace musicplayer {

class LyricWidget : public QWidget {
    Q_OBJECT
public:
    explicit LyricWidget(QWidget* parent = nullptr);
};

} // namespace musicplayer
