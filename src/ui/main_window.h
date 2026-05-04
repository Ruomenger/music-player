#pragma once

#include <QMainWindow>

namespace musicplayer {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
};

}  // namespace musicplayer
