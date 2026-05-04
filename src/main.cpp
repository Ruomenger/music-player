#include <QApplication>
#include "ui/main_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Music Player");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("musicplayer");

    musicplayer::MainWindow window;
    window.show();

    return app.exec();
}
