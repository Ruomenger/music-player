#pragma once

#include <QPixmap>
#include <QWidget>

namespace musicplayer {

// Displays a square album-cover image scaled to the widget's current size.
// setCoverPath("") (or a missing file) shows a programmatically-painted
// placeholder so the layout doesn't collapse.
class CoverWidget : public QWidget {
    Q_OBJECT
public:
    explicit CoverWidget(QWidget* parent = nullptr);

    void setCoverPath(const QString& path);
    [[nodiscard]] bool hasCover() const { return !pixmap_.isNull(); }

    [[nodiscard]] QSize sizeHint() const override { return {200, 200}; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    static QPixmap makePlaceholder();

    QPixmap pixmap_;
    QPixmap placeholder_;
};

}  // namespace musicplayer
