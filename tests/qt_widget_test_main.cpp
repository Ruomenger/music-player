#include <gtest/gtest.h>

#include <QApplication>
#include <QByteArray>

// QApplication-backed entry point for UI tests. Forces the offscreen platform
// plugin so the suite runs headlessly on CI / SSH sessions (where there's no
// display server). Local devs can override by exporting QT_QPA_PLATFORM
// before invoking ctest.
int main(int argc, char** argv) {
    // Leave any user-supplied QT_QPA_PLATFORM untouched; qputenv() always
    // overwrites, so gate it on the existing value.
    if (!qEnvironmentVariableIsSet("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
