#pragma once

#include <QImage>
#include <QString>
#include <vector>

#include "layer.h"

// Reader for Paint.NET's native layered format (.pdn).
//
// A .pdn file is "PDN3", a 24-bit little-endian length, an XML header (size and
// thumbnail), the bytes 00 01, then the Document object graph written by .NET's
// BinaryFormatter (MS-NRBF). Layer pixels are not part of that graph: each
// surface's MemoryBlock is marked "deferred" and its BGRA bytes follow the NRBF
// stream as gzip-compressed (or raw) chunks, in serialization order.
namespace Pdn {

struct LayerData {
    QImage image;               // Format_ARGB32 (straight alpha, like Paint.NET)
    QString name;
    bool visible = true;
    float opacity = 1.0f;
    BlendMode blendMode = BlendMode::Normal;
};

struct ImageData {
    int width = 0;
    int height = 0;
    std::vector<LayerData> layers;   // bottom (background) first
};

// True when the file starts with the "PDN3" signature.
bool isPdnFile(const QString &filePath);

bool read(const QString &filePath, ImageData &out, QString *error = nullptr);
bool read(const QByteArray &data, ImageData &out, QString *error = nullptr);

} // namespace Pdn
