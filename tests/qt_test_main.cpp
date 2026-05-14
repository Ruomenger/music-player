#include <gtest/gtest.h>

#include <QCoreApplication>

// QSqlDatabase / QStandardPaths and friends need a live QCoreApplication for
// their plugin / settings infrastructure. The default GTest::gtest_main lacks
// one, so SQL tests link against this main instead.
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
