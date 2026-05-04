#pragma once

#include <QWidget>

namespace musicplayer {

class CoverWidget : public QWidget {
    Q_OBJECT
public:
    explicit CoverWidget(QWidget* parent = nullptr);
};

}  // namespace musicplayer
