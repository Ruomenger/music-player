#include <gtest/gtest.h>

#include <QApplication>

#include <cstdlib>

// QApplication-backed entry point for UI tests. Forces the offscreen platform
// plugin so the suite runs headlessly on CI / SSH sessions (where there's no
// display server). Local devs can override by exporting QT_QPA_PLATFORM
// before invoking ctest.
int main(int argc, char** argv) {
    // setenv(name, value, overwrite=0) leaves an existing user value intact.
    // The lint warns this isn't thread-safe; there is no other thread before
    // QApplication is constructed, so the warning doesn't apply.
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    ::setenv("QT_QPA_PLATFORM", "offscreen", /*overwrite=*/0);
    QApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
