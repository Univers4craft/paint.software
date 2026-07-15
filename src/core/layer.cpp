#include "layer.h"
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <cstdlib>

Layer::Layer(int width, int height, const QString &name)
    : m_image(width, height, QImage::Format_ARGB32_Premultiplied)
    , m_name(name)
{
    m_image.fill(Qt::transparent);
}

Layer::Layer(const QImage &image, const QString &name)
    : m_image(image.convertToFormat(QImage::Format_ARGB32_Premultiplied))
    , m_name(name)
{
}

Layer::Layer(const Layer &other)
    : m_image(other.m_image)
    , m_name(other.m_name)
    , m_opacity(other.m_opacity)
    , m_visible(other.m_visible)
    , m_locked(other.m_locked)
    , m_blendMode(other.m_blendMode)
    , m_offset(other.m_offset)
{
}

Layer &Layer::operator=(const Layer &other) {
    if (this != &other) {
        m_image = other.m_image;
        m_name = other.m_name;
        m_opacity = other.m_opacity;
        m_visible = other.m_visible;
        m_locked = other.m_locked;
        m_blendMode = other.m_blendMode;
        m_offset = other.m_offset;
    }
    return *this;
}

void Layer::resize(int width, int height) {
    QImage newImage(width, height, QImage::Format_ARGB32_Premultiplied);
    newImage.fill(Qt::transparent);
    QPainter painter(&newImage);
    painter.drawImage(0, 0, m_image);
    m_image = newImage;
}

void Layer::clear(const QColor &color) {
    m_image.fill(color);
}

QImage Layer::composited(const QImage &below) const {
    if (!m_visible || m_opacity <= 0.0f)
        return below;

    // Paint.NET blend modes that Qt has no built-in composition mode for are
    // computed per-pixel here, then composited SourceOver.
    if (m_blendMode == BlendMode::Glow || m_blendMode == BlendMode::Reflect
        || m_blendMode == BlendMode::Negation) {
        QImage base = below.convertToFormat(QImage::Format_ARGB32);
        QImage top = m_image.convertToFormat(QImage::Format_ARGB32);
        // "blended" holds the blend result with the top layer's alpha; drawn
        // SourceOver so transparent top pixels leave the base untouched.
        QImage blended(base.size(), QImage::Format_ARGB32);
        blended.fill(Qt::transparent);

        auto reflect = [](int b, int s) -> int {
            if (s >= 255) return 255;
            return std::min(255, b * b / (255 - s));
        };

        for (int y = 0; y < base.height(); ++y) {
            int ty = y - m_offset.y();
            if (ty < 0 || ty >= top.height()) continue;
            const QRgb *topLine = reinterpret_cast<const QRgb*>(top.constScanLine(ty));
            const QRgb *baseLine = reinterpret_cast<const QRgb*>(base.constScanLine(y));
            QRgb *outLine = reinterpret_cast<QRgb*>(blended.scanLine(y));
            for (int x = 0; x < base.width(); ++x) {
                int tx = x - m_offset.x();
                if (tx < 0 || tx >= top.width()) continue;
                QRgb s = topLine[tx];
                if (qAlpha(s) == 0) continue;
                QRgb b = baseLine[x];
                int rr, gg, bb;
                switch (m_blendMode) {
                case BlendMode::Reflect:
                    rr = reflect(qRed(b), qRed(s));
                    gg = reflect(qGreen(b), qGreen(s));
                    bb = reflect(qBlue(b), qBlue(s));
                    break;
                case BlendMode::Glow:  // inverse of reflect (base/source swapped)
                    rr = reflect(qRed(s), qRed(b));
                    gg = reflect(qGreen(s), qGreen(b));
                    bb = reflect(qBlue(s), qBlue(b));
                    break;
                case BlendMode::Negation:
                default:
                    rr = 255 - std::abs(255 - qRed(b) - qRed(s));
                    gg = 255 - std::abs(255 - qGreen(b) - qGreen(s));
                    bb = 255 - std::abs(255 - qBlue(b) - qBlue(s));
                    break;
                }
                // Where the base is (partly) transparent the blend has nothing to
                // work against, so fade toward the source colour by the base alpha.
                // Otherwise these modes render black fringes over transparent areas.
                const double bA = qAlpha(b) / 255.0;
                rr = static_cast<int>(qRed(s)   * (1.0 - bA) + rr * bA + 0.5);
                gg = static_cast<int>(qGreen(s) * (1.0 - bA) + gg * bA + 0.5);
                bb = static_cast<int>(qBlue(s)  * (1.0 - bA) + bb * bA + 0.5);
                outLine[x] = qRgba(qBound(0, rr, 255), qBound(0, gg, 255),
                                   qBound(0, bb, 255), qAlpha(s));
            }
        }

        QImage result = below.copy();
        QPainter painter(&result);
        painter.setOpacity(m_opacity);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, blended.convertToFormat(QImage::Format_ARGB32_Premultiplied));
        return result;
    }

    QImage result = below.copy();
    QPainter painter(&result);
    painter.setOpacity(m_opacity);

    switch (m_blendMode) {
    case BlendMode::Normal:
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        break;
    case BlendMode::Multiply:
        painter.setCompositionMode(QPainter::CompositionMode_Multiply);
        break;
    case BlendMode::Additive:
        painter.setCompositionMode(QPainter::CompositionMode_Plus);
        break;
    case BlendMode::ColorBurn:
        painter.setCompositionMode(QPainter::CompositionMode_ColorBurn);
        break;
    case BlendMode::ColorDodge:
        painter.setCompositionMode(QPainter::CompositionMode_ColorDodge);
        break;
    case BlendMode::Darken:
        painter.setCompositionMode(QPainter::CompositionMode_Darken);
        break;
    case BlendMode::Difference:
        painter.setCompositionMode(QPainter::CompositionMode_Difference);
        break;
    case BlendMode::Glow:
        painter.setCompositionMode(QPainter::CompositionMode_ColorDodge);
        break;
    case BlendMode::Lighten:
        painter.setCompositionMode(QPainter::CompositionMode_Lighten);
        break;
    case BlendMode::Negation:
        painter.setCompositionMode(QPainter::CompositionMode_Difference);
        break;
    case BlendMode::Overlay:
        painter.setCompositionMode(QPainter::CompositionMode_Overlay);
        break;
    case BlendMode::Reflect:
        painter.setCompositionMode(QPainter::CompositionMode_Screen);
        break;
    case BlendMode::Screen:
        painter.setCompositionMode(QPainter::CompositionMode_Screen);
        break;
    case BlendMode::Xor:
        painter.setCompositionMode(QPainter::CompositionMode_Xor);
        break;
    case BlendMode::Overwrite:
        painter.setCompositionMode(QPainter::CompositionMode_Source);
        break;
    }

    painter.drawImage(m_offset, m_image);
    return result;
}

QString Layer::blendModeName(BlendMode mode) {
    switch (mode) {
    case BlendMode::Normal: return "Normal";
    case BlendMode::Multiply: return "Multiply";
    case BlendMode::Additive: return "Additive";
    case BlendMode::ColorBurn: return "Color Burn";
    case BlendMode::ColorDodge: return "Color Dodge";
    case BlendMode::Darken: return "Darken";
    case BlendMode::Difference: return "Difference";
    case BlendMode::Glow: return "Glow";
    case BlendMode::Lighten: return "Lighten";
    case BlendMode::Negation: return "Negation";
    case BlendMode::Overlay: return "Overlay";
    case BlendMode::Reflect: return "Reflect";
    case BlendMode::Screen: return "Screen";
    case BlendMode::Xor: return "Xor";
    case BlendMode::Overwrite: return "Overwrite";
    }
    return "Normal";
}

QList<BlendMode> Layer::allBlendModes() {
    return {
        BlendMode::Normal, BlendMode::Multiply, BlendMode::Additive,
        BlendMode::ColorBurn, BlendMode::ColorDodge, BlendMode::Darken,
        BlendMode::Difference, BlendMode::Glow, BlendMode::Lighten,
        BlendMode::Negation, BlendMode::Overlay, BlendMode::Reflect,
        BlendMode::Screen, BlendMode::Xor, BlendMode::Overwrite
    };
}
