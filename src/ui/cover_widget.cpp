#include "cover_widget.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QString>

namespace musicplayer {

CoverWidget::CoverWidget(QWidget* parent) : QWidget(parent), placeholder_(makePlaceholder()) {
    setMinimumSize(120, 120);
}

void CoverWidget::setCoverPath(const QString& path) {
    if (path.isEmpty()) {
        pixmap_ = QPixmap();
    } else {
        QPixmap loaded(path);
        // QPixmap is null when the path doesn't exist or the format isn't
        // recognised. Fall through to the placeholder in that case.
        pixmap_ = loaded.isNull() ? QPixmap() : loaded;
    }
    update();
}

void CoverWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const QPixmap& source = pixmap_.isNull() ? placeholder_ : pixmap_;
    const QSize target = event->rect().size();
    const QPixmap scaled = source.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // Center the scaled pixmap inside the widget rectangle.
    const int x = (target.width() - scaled.width()) / 2;
    const int y = (target.height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
}

QPixmap CoverWidget::makePlaceholder() {
    // Programmatic placeholder — flat fill + centered glyph. We don't ship
    // image assets yet (Phase 8 will add resources/icons), so generate one.
    constexpr int kSide = 200;
    QPixmap pm(kSide, kSide);
    pm.fill(QColor(80, 80, 90));
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing);
    QFont font = painter.font();
    font.setPointSize(72);
    painter.setFont(font);
    painter.setPen(QPen(QColor(200, 200, 210)));
    painter.drawText(pm.rect(), Qt::AlignCenter, QStringLiteral("♪"));
    return pm;
}

}  // namespace musicplayer
