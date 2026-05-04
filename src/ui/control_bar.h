#pragma once

#include <QWidget>

namespace musicplayer {

class ControlBar : public QWidget {
    Q_OBJECT
public:
    explicit ControlBar(QWidget* parent = nullptr);
};

} // namespace musicplayer
