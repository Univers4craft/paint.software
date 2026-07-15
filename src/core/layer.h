#pragma once

#include <QImage>
#include <QString>
#include <QPainter>

enum class BlendMode {
    Normal,
    Multiply,
    Additive,
    ColorBurn,
    ColorDodge,
    Darken,
    Difference,
    Glow,
    Lighten,
    Negation,
    Overlay,
    Reflect,
    Screen,
    Xor,
    Overwrite
};

class Layer {
public:
    Layer(int width, int height, const QString &name = "Layer");
    Layer(const QImage &image, const QString &name = "Layer");
    Layer(const Layer &other);
    Layer &operator=(const Layer &other);

    const QImage &image() const { return m_image; }
    QImage &image() { return m_image; }
    // Always store premultiplied ARGB32 so the layer invariant holds even when an
    // adjustment/effect hands back a straight-alpha (Format_ARGB32) image.
    void setImage(const QImage &image) {
        m_image = image.format() == QImage::Format_ARGB32_Premultiplied
                      ? image
                      : image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }

    QString name() const { return m_name; }
    void setName(const QString &name) { m_name = name; }

    float opacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = qBound(0.0f, opacity, 1.0f); }

    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }

    bool isLocked() const { return m_locked; }
    void setLocked(bool locked) { m_locked = locked; }

    BlendMode blendMode() const { return m_blendMode; }
    void setBlendMode(BlendMode mode) { m_blendMode = mode; }

    QPoint offset() const { return m_offset; }
    void setOffset(const QPoint &offset) { m_offset = offset; }

    int width() const { return m_image.width(); }
    int height() const { return m_image.height(); }

    void resize(int width, int height);
    void clear(const QColor &color = Qt::transparent);
    QImage composited(const QImage &below) const;

    static QString blendModeName(BlendMode mode);
    static QList<BlendMode> allBlendModes();

private:
    QImage m_image;
    QString m_name;
    float m_opacity = 1.0f;
    bool m_visible = true;
    bool m_locked = false;
    BlendMode m_blendMode = BlendMode::Normal;
    QPoint m_offset{0, 0};
};
