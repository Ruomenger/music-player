#include "lyric_widget.h"

#include <QAbstractItemView>
#include <QFileDialog>
#include <QFont>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>

namespace musicplayer {

LyricWidget::LyricWidget(QWidget* parent)
    : QWidget(parent)
    , list_(new QListWidget(this))
    , loadBtn_(new QPushButton(tr("Load lyrics…"), this)) {
    list_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list_->setSelectionMode(QAbstractItemView::NoSelection);
    list_->setFocusPolicy(Qt::NoFocus);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(list_, /*stretch=*/1);
    layout->addWidget(loadBtn_);

    connect(loadBtn_, &QPushButton::clicked, this, &LyricWidget::onLoadClicked);
}

void LyricWidget::setLines(const std::vector<LyricLine>& lines) {
    lines_ = lines;
    currentIndex_ = -1;
    rebuildList();
}

void LyricWidget::setCurrentLine(int index) {
    if (index == currentIndex_)
        return;
    applyHighlight(index);
    currentIndex_ = index;
    // Center the active line in the viewport so users always see context
    // above and below. PositionAtCenter is the only scroll hint that gives a
    // consistent visual cadence as the cursor advances line by line.
    if (index >= 0 && index < list_->count()) {
        list_->scrollToItem(list_->item(index), QAbstractItemView::PositionAtCenter);
    }
}

void LyricWidget::onLoadClicked() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Load lyrics"), QString(),
                                                      tr("LRC files (*.lrc);;All files (*)"));
    if (!path.isEmpty())
        emit manualLyricsRequested(path);
}

void LyricWidget::rebuildList() {
    list_->clear();
    if (lines_.empty()) {
        // Single placeholder row so the empty pane reads as intentional
        // rather than broken.
        auto* item = new QListWidgetItem(tr("(no lyrics)"), list_);
        item->setTextAlignment(Qt::AlignCenter);
        return;
    }
    for (const auto& line : lines_) {
        auto* item = new QListWidgetItem(
            line.text.empty() ? QStringLiteral(" ") : QString::fromStdString(line.text), list_);
        item->setTextAlignment(Qt::AlignCenter);
    }
}

void LyricWidget::applyHighlight(int index) {
    // Reset the previous active row to plain weight first.
    if (currentIndex_ >= 0 && currentIndex_ < list_->count()) {
        auto* prev = list_->item(currentIndex_);
        QFont f = prev->font();
        f.setBold(false);
        prev->setFont(f);
    }
    if (index >= 0 && index < list_->count()) {
        auto* cur = list_->item(index);
        QFont f = cur->font();
        f.setBold(true);
        cur->setFont(f);
    }
}

}  // namespace musicplayer
