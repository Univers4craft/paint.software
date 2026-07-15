#pragma once

#include <QImage>
#include <QString>

class Effect {
public:
    virtual ~Effect() = default;
    virtual QString name() const = 0;
    virtual QImage apply(const QImage &input) = 0;
};
