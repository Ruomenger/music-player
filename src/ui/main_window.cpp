#include "main_window.h"
#include "constants.h"

#include <QLabel>

namespace musicplayer {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QString::fromUtf8(APP_NAME.data(), static_cast<int>(APP_NAME.size())));
    setMinimumSize(800, 600);

    auto* centralLabel =
        new QLabel(QString::fromUtf8(APP_NAME.data(), static_cast<int>(APP_NAME.size())), this);
    centralLabel->setAlignment(Qt::AlignCenter);
    QFont font = centralLabel->font();
    font.setPointSize(24);
    centralLabel->setFont(font);
    setCentralWidget(centralLabel);
}

}  // namespace musicplayer
