#pragma once
#include <QDialog>
#include <QSpinBox>
#include <QToolButton>
#include <QButtonGroup>

class CanvasSizeDialog : public QDialog {
    Q_OBJECT
public:
    explicit CanvasSizeDialog(int currentWidth, int currentHeight, QWidget *parent = nullptr);
    int newWidth() const;
    int newHeight() const;
    int anchorX() const { return m_anchorX; }
    int anchorY() const { return m_anchorY; }

private slots:
    void onAnchorClicked(int id);
    void updateAnchor();

private:
    QSpinBox *m_widthSpin;
    QSpinBox *m_heightSpin;
    QButtonGroup *m_anchorGroup;
    QToolButton *m_anchorButtons[9];
    int m_anchorX = 0;
    int m_anchorY = 0;
    int m_anchorIndex = 0;
    int m_currentWidth;
    int m_currentHeight;
};
