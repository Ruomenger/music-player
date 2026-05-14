#pragma once

#include <QWidget>

#include <vector>

#include "lyric_parser.h"

class QListWidget;
class QPushButton;

namespace musicplayer {

// Lyric pane: a vertical scrollable list of timestamped lines plus a
// "Load lyrics…" button for manual override. Reflects the LyricManager's
// state via setLines / setCurrentLine; the latter scrolls + bolds the
// active row. Emits manualLyricsRequested when the user picks a file from
// the file dialog so the host can hand it back to the manager.
class LyricWidget : public QWidget {
    Q_OBJECT
public:
    explicit LyricWidget(QWidget* parent = nullptr);

    [[nodiscard]] int currentLineIndex() const { return currentIndex_; }
    [[nodiscard]] int lineCount() const { return static_cast<int>(lines_.size()); }

public slots:
    void setLines(const std::vector<LyricLine>& lines);
    void setCurrentLine(int index);  // -1 = before first line

signals:
    void manualLyricsRequested(const QString& path);

private slots:
    void onLoadClicked();

private:
    void rebuildList();
    void applyHighlight(int index);

    std::vector<LyricLine> lines_;
    int currentIndex_ = -1;
    QListWidget* list_;
    QPushButton* loadBtn_;
};

}  // namespace musicplayer
