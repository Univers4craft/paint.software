#pragma once

#include <QDialog>
#include <QVector>
#include <QPointF>

// Interactive tone-curve editor. The curve maps input [0..255] to output
// [0..255]; drag control points, click empty space to add one, right-click a
// point to remove it.
class CurveWidget;

class CurvesDialog : public QDialog {
    Q_OBJECT
public:
    explicit CurvesDialog(QWidget *parent = nullptr);
    // Control points in 0..255 space, sorted by x.
    QVector<QPointF> controlPoints() const;

signals:
    void curveChanged();

private:
    CurveWidget *m_curve;
};
