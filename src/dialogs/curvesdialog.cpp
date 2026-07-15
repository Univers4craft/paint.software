#include "curvesdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QPainter>
#include <QMouseEvent>
#include <QLineF>
#include <functional>
#include <algorithm>

// ---- Interactive curve canvas ----
class CurveWidget : public QWidget {
public:
    explicit CurveWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(256, 256);
        setMouseTracking(true);
        m_points = { QPointF(0, 0), QPointF(255, 255) };
    }

    QVector<QPointF> points() const { return m_points; }

    void reset() {
        m_points = { QPointF(0, 0), QPointF(255, 255) };
        update();
        if (onChanged) onChanged();
    }

    std::function<void()> onChanged;

protected:
    QPointF toWidget(const QPointF &p) const {
        return QPointF(p.x(), 255.0 - p.y());
    }
    QPointF toCurve(const QPointF &p) const {
        return QPointF(qBound(0.0, p.x(), 255.0), qBound(0.0, 255.0 - p.y(), 255.0));
    }

    int hitTest(const QPointF &widgetPos) const {
        for (int i = 0; i < m_points.size(); ++i) {
            if (QLineF(toWidget(m_points[i]), widgetPos).length() < 8.0)
                return i;
        }
        return -1;
    }

    void mousePressEvent(QMouseEvent *e) override {
        if (e->button() == Qt::RightButton) {
            int i = hitTest(e->position());
            if (i > 0 && i < m_points.size() - 1) {  // can't delete endpoints
                m_points.remove(i);
                update();
                if (onChanged) onChanged();
            }
            return;
        }
        m_dragIndex = hitTest(e->position());
        if (m_dragIndex < 0) {
            // Add a new point.
            QPointF cp = toCurve(e->position());
            m_points.append(cp);
            std::sort(m_points.begin(), m_points.end(),
                      [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
            m_dragIndex = hitTest(e->position());
            update();
            if (onChanged) onChanged();
        }
    }

    void mouseMoveEvent(QMouseEvent *e) override {
        if (m_dragIndex < 0) return;
        QPointF cp = toCurve(e->position());
        // Endpoints keep their x fixed at 0 / 255.
        if (m_dragIndex == 0) cp.setX(0);
        else if (m_dragIndex == m_points.size() - 1) cp.setX(255);
        else {
            double lo = m_points[m_dragIndex - 1].x() + 1;
            double hi = m_points[m_dragIndex + 1].x() - 1;
            cp.setX(qBound(lo, cp.x(), hi));
        }
        m_points[m_dragIndex] = cp;
        update();
        if (onChanged) onChanged();
    }

    void mouseReleaseEvent(QMouseEvent *) override { m_dragIndex = -1; }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.fillRect(rect(), QColor(30, 30, 30));
        p.setPen(QPen(QColor(70, 70, 70), 1));
        for (int i = 1; i < 4; ++i) {
            int v = i * 64;
            p.drawLine(v, 0, v, 255);
            p.drawLine(0, v, 255, v);
        }
        // Diagonal reference.
        p.setPen(QPen(QColor(90, 90, 90), 1, Qt::DashLine));
        p.drawLine(0, 255, 255, 0);

        // Curve using the same linear interpolation the Curves adjustment uses.
        p.setPen(QPen(QColor(120, 200, 255), 2));
        QPointF prev = toWidget(QPointF(0, evalAt(0)));
        for (int x = 1; x <= 255; ++x) {
            QPointF cur = toWidget(QPointF(x, evalAt(x)));
            p.drawLine(prev, cur);
            prev = cur;
        }

        // Control points.
        p.setBrush(Qt::white);
        p.setPen(QPen(Qt::black, 1));
        for (const QPointF &cp : m_points)
            p.drawEllipse(toWidget(cp), 4, 4);
    }

    double evalAt(int x) const {
        // Piecewise-linear between sorted control points.
        for (int j = 0; j < m_points.size() - 1; ++j) {
            if (m_points[j].x() <= x && m_points[j + 1].x() >= x) {
                double range = m_points[j + 1].x() - m_points[j].x();
                double t = (range > 0) ? (x - m_points[j].x()) / range : 0;
                return m_points[j].y() + t * (m_points[j + 1].y() - m_points[j].y());
            }
        }
        return x;
    }

private:
    QVector<QPointF> m_points;
    int m_dragIndex = -1;
};

// ---- Dialog ----
CurvesDialog::CurvesDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Courbes");
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Glissez les points. Clic pour ajouter, clic droit pour supprimer."));

    m_curve = new CurveWidget(this);
    m_curve->onChanged = [this]() { emit curveChanged(); };
    auto *curveRow = new QHBoxLayout;
    curveRow->addStretch();
    curveRow->addWidget(m_curve);
    curveRow->addStretch();
    layout->addLayout(curveRow);

    auto *btnRow = new QHBoxLayout;
    auto *resetBtn = new QPushButton("Réinitialiser");
    connect(resetBtn, &QPushButton::clicked, m_curve, [this]() { m_curve->reset(); });
    auto *okBtn = new QPushButton("OK");
    okBtn->setDefault(true);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    auto *cancelBtn = new QPushButton("Annuler");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(resetBtn);
    btnRow->addStretch();
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addLayout(btnRow);
}

QVector<QPointF> CurvesDialog::controlPoints() const {
    return m_curve->points();
}
