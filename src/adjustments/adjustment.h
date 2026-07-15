#pragma once
#include <QImage>
#include <QString>

class Adjustment {
public:
    virtual ~Adjustment() = default;
    virtual QString name() const = 0;
    virtual QImage apply(const QImage &input) = 0;
};
