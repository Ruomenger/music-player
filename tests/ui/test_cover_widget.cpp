#include <gtest/gtest.h>
#include "cover_widget.h"

#include <QImage>
#include <QString>
#include <QTemporaryDir>

using musicplayer::CoverWidget;

TEST(CoverWidgetTest, EmptyPathReportsNoCover) {
    CoverWidget w;
    EXPECT_FALSE(w.hasCover());
    w.setCoverPath(QString());
    EXPECT_FALSE(w.hasCover());
}

TEST(CoverWidgetTest, MissingFileFallsBackToPlaceholder) {
    CoverWidget w;
    w.setCoverPath(QStringLiteral("/definitely/not/a/file.png"));
    EXPECT_FALSE(w.hasCover());  // null pixmap → placeholder will render
}

TEST(CoverWidgetTest, LoadsValidImage) {
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("cover.png"));

    QImage img(40, 40, QImage::Format_RGB32);
    img.fill(Qt::red);
    ASSERT_TRUE(img.save(path));

    CoverWidget w;
    w.setCoverPath(path);
    EXPECT_TRUE(w.hasCover());
}
