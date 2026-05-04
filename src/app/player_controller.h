#pragma once

#include <QObject>
#include <memory>

namespace musicplayer {

class PlayerController : public QObject {
    Q_OBJECT
public:
    explicit PlayerController(QObject* parent = nullptr);

signals:
    void positionChanged(double current, double total);
};

}  // namespace musicplayer
