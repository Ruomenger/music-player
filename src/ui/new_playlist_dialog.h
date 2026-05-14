#pragma once

#include <QDialog>
#include <QString>

#include <optional>

class QLineEdit;

namespace musicplayer {

// Modal dialog that collects a new playlist's name and (optional) description.
// Use the static run() helper at call sites — returns the entered values on
// Accept, nullopt on cancel.
class NewPlaylistDialog : public QDialog {
    Q_OBJECT
public:
    struct Result {
        QString name;
        QString description;
    };

    explicit NewPlaylistDialog(QWidget* parent = nullptr);

    [[nodiscard]] QString playlistName() const;
    [[nodiscard]] QString playlistDescription() const;

    // Convenience helper: shows the dialog modally and returns the entered
    // values, or nullopt if the user cancelled / left the name blank.
    [[nodiscard]] static std::optional<Result> run(QWidget* parent);

private slots:
    void onAccept();

private:
    QLineEdit* nameEdit_;
    QLineEdit* descEdit_;
};

}  // namespace musicplayer
