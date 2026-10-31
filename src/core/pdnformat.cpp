#include "pdnformat.h"

#include <QFile>
#include <QHash>
#include <QStringList>
#include <cstring>
#include <iterator>
#include <limits>
#include <zlib.h>

namespace Pdn {
namespace {

// ---------------------------------------------------------------------------
// Minimal MS-NRBF (.NET BinaryFormatter) reader. It builds a flat table of
// objects keyed by id; references stay as ids and are resolved on lookup.
// Only what a Paint.NET Document needs is interpreted, but every record type
// that can appear in a plain object graph is parsed so we find where the
// stream ends (the deferred pixel data starts right after MessageEnd).
// ---------------------------------------------------------------------------

enum RecordType : quint8 {
    SerializedStreamHeader = 0,
    ClassWithId = 1,
    SystemClassWithMembers = 2,
    ClassWithMembers = 3,
    SystemClassWithMembersAndTypes = 4,
    ClassWithMembersAndTypes = 5,
    BinaryObjectString = 6,
    BinaryArray = 7,
    MemberPrimitiveTyped = 8,
    MemberReference = 9,
    ObjectNull = 10,
    MessageEnd = 11,
    BinaryLibrary = 12,
    ObjectNullMultiple256 = 13,
    ObjectNullMultiple = 14,
    ArraySinglePrimitive = 15,
    ArraySingleObject = 16,
    ArraySingleString = 17,
};

enum BinaryType : quint8 {
    BtPrimitive = 0, BtString = 1, BtObject = 2, BtSystemClass = 3,
    BtClass = 4, BtObjectArray = 5, BtStringArray = 6, BtPrimitiveArray = 7,
};

enum PrimitiveType : quint8 {
    PtBoolean = 1, PtByte = 2, PtChar = 3, PtDecimal = 5, PtDouble = 6,
    PtInt16 = 7, PtInt32 = 8, PtInt64 = 9, PtSByte = 10, PtSingle = 11,
    PtTimeSpan = 12, PtDateTime = 13, PtUInt16 = 14, PtUInt32 = 15,
    PtUInt64 = 16, PtNull = 17, PtString = 18,
};

constexpr int kMaxDepth = 128;
constexpr qint64 kMaxElements = 1 << 22;   // object-array elements / class members

struct Value {
    enum Kind { Null, Int, Double, String, Ref, NullRun, Library, End } kind = Null;
    qint64 i = 0;       // Int / Bool / Char / NullRun count / Ref id
    double d = 0.0;
    QString s;
};

struct ClassMeta {
    QString name;
    QStringList members;
    bool hasTypes = false;
    std::vector<quint8> binaryTypes;
    std::vector<quint8> primitiveTypes;   // valid where binaryType is Primitive
};

struct Object {
    QString className;                    // "[]" for arrays
    QStringList memberNames;
    std::vector<Value> values;            // members, or array elements
    QByteArray bytes;                     // byte[] contents
};

class Error {};

class NrbfReader {
public:
    NrbfReader(const QByteArray &data, qint64 pos) : m_data(data), m_pos(pos) {}

    void parse() {
        if (readByte() != SerializedStreamHeader) throw Error();
        m_rootId = readInt32();
        readInt32(); readInt32(); readInt32();   // header id, major, minor
        for (;;) {
            Value v = readRecord(0);
            if (v.kind == Value::End) break;
        }
    }

    qint64 position() const { return m_pos; }
    qint32 rootId() const { return m_rootId; }
    const QHash<qint32, Object> &objects() const { return m_objects; }
    const std::vector<qint32> &order() const { return m_order; }

    const Object *deref(const Value *v) const {
        if (!v || v->kind != Value::Ref) return nullptr;
        auto it = m_objects.constFind(static_cast<qint32>(v->i));
        return it == m_objects.constEnd() ? nullptr : &it.value();
    }
    QString stringOf(const Value *v) const {
        if (!v) return {};
        if (v->kind == Value::String) return v->s;
        if (v->kind == Value::Ref) return m_strings.value(static_cast<qint32>(v->i));
        return {};
    }

private:
    void need(qint64 n) const {
        if (n < 0 || m_pos + n > m_data.size()) throw Error();
    }
    quint8 readByte() { need(1); return static_cast<quint8>(m_data[m_pos++]); }
    template <typename T> T readLE() {
        need(sizeof(T));
        T v;
        std::memcpy(&v, m_data.constData() + m_pos, sizeof(T));   // NRBF is little-endian
        m_pos += sizeof(T);
        return v;
    }
    qint32 readInt32() { return readLE<qint32>(); }
    qint32 readCount() {
        const qint32 n = readInt32();
        if (n < 0) throw Error();
        return n;
    }

    QString readString() {
        quint32 len = 0;
        for (int shift = 0;; shift += 7) {
            if (shift > 28) throw Error();
            const quint8 b = readByte();
            len |= quint32(b & 0x7f) << shift;
            if (!(b & 0x80)) break;
        }
        need(len);
        QString s = QString::fromUtf8(m_data.constData() + m_pos, static_cast<int>(len));
        m_pos += len;
        return s;
    }

    Value readPrimitive(quint8 type) {
        Value v;
        v.kind = Value::Int;
        switch (type) {
        case PtBoolean: case PtByte: v.i = readByte(); break;
        case PtSByte:   v.i = static_cast<qint8>(readByte()); break;
        case PtChar: {
            // One UTF-8 encoded character; its lead byte gives the length.
            const quint8 lead = readByte();
            const int extra = lead >= 0xf0 ? 3 : lead >= 0xe0 ? 2 : lead >= 0xc0 ? 1 : 0;
            need(extra);
            v.kind = Value::String;
            v.s = QString::fromUtf8(m_data.constData() + m_pos - 1, extra + 1);
            m_pos += extra;
            break;
        }
        case PtInt16:   v.i = readLE<qint16>(); break;
        case PtUInt16:  v.i = readLE<quint16>(); break;
        case PtInt32:   v.i = readLE<qint32>(); break;
        case PtUInt32:  v.i = readLE<quint32>(); break;
        case PtInt64:   v.i = readLE<qint64>(); break;
        case PtUInt64:  v.i = static_cast<qint64>(readLE<quint64>()); break;
        case PtTimeSpan: case PtDateTime: v.i = readLE<qint64>(); break;
        case PtSingle:  v.kind = Value::Double; v.d = readLE<float>(); break;
        case PtDouble:  v.kind = Value::Double; v.d = readLE<double>(); break;
        case PtDecimal: v.kind = Value::String; v.s = readString(); break;
        case PtString:  v.kind = Value::String; v.s = readString(); break;
        case PtNull:    v.kind = Value::Null; break;
        default: throw Error();
        }
        return v;
    }

    static int primitiveSize(quint8 type) {
        switch (type) {
        case PtBoolean: case PtByte: case PtSByte: return 1;
        case PtInt16: case PtUInt16: return 2;
        case PtInt32: case PtUInt32: case PtSingle: return 4;
        case PtInt64: case PtUInt64: case PtDouble: case PtTimeSpan: case PtDateTime: return 8;
        default: return 1;   // variable-length (char, decimal, string): at least one byte
        }
    }

    // Reads the additional type info that follows each BinaryType byte.
    quint8 readAdditionalInfo(quint8 binaryType) {
        switch (binaryType) {
        case BtPrimitive: case BtPrimitiveArray: return readByte();
        case BtSystemClass: readString(); return 0;
        case BtClass: readString(); readInt32(); return 0;
        case BtString: case BtObject: case BtObjectArray: case BtStringArray: return 0;
        default: throw Error();
        }
    }

    ClassMeta readClassInfo(qint32 &objectId) {
        ClassMeta meta;
        objectId = readInt32();
        meta.name = readString();
        const qint32 count = readCount();
        if (count > kMaxElements || count > m_data.size() - m_pos) throw Error();
        for (qint32 i = 0; i < count; ++i) meta.members << readString();
        return meta;
    }

    void readMemberTypeInfo(ClassMeta &meta) {
        meta.hasTypes = true;
        const int n = meta.members.size();
        meta.binaryTypes.resize(n);
        meta.primitiveTypes.assign(n, 0);
        for (int i = 0; i < n; ++i) meta.binaryTypes[i] = readByte();
        for (int i = 0; i < n; ++i) meta.primitiveTypes[i] = readAdditionalInfo(meta.binaryTypes[i]);
    }

    // Reads `count` member values or array elements. Without type info each
    // value is a full record; null runs fill several slots at once.
    void readValues(std::vector<Value> &out, qint64 count, const ClassMeta *meta,
                    int depth, bool elementsArePrimitive = false, quint8 elementType = 0) {
        out.reserve(static_cast<size_t>(count));
        while (static_cast<qint64>(out.size()) < count) {
            const size_t index = out.size();
            const bool primitive = elementsArePrimitive
                || (meta && meta->hasTypes && meta->binaryTypes[index] == BtPrimitive);
            if (primitive) {
                out.push_back(readPrimitive(elementsArePrimitive ? elementType
                                                                 : meta->primitiveTypes[index]));
                continue;
            }
            Value v = readRecord(depth + 1);
            if (v.kind == Value::Library) continue;
            if (v.kind == Value::End) throw Error();
            if (v.kind == Value::NullRun) {
                if (v.i > count - static_cast<qint64>(out.size())) throw Error();
                out.insert(out.end(), static_cast<size_t>(v.i), Value());
                continue;
            }
            out.push_back(v);
        }
    }

    Value refTo(qint32 id) {
        Value v;
        v.kind = Value::Ref;
        v.i = id;
        return v;
    }

    Value storeClass(qint32 id, const ClassMeta &meta, int depth) {
        if (m_objects.contains(id)) throw Error();
        m_classes.insert(id, meta);
        Object obj;
        obj.className = meta.name;
        obj.memberNames = meta.members;
        readValues(obj.values, meta.members.size(), &meta, depth);
        m_objects.insert(id, std::move(obj));
        m_order.push_back(id);
        return refTo(id);
    }

    Value storeArray(qint32 id, Object &&obj) {
        if (m_objects.contains(id)) throw Error();
        obj.className = QStringLiteral("[]");
        m_objects.insert(id, std::move(obj));
        m_order.push_back(id);
        return refTo(id);
    }

    void readPrimitiveArray(Object &obj, qint64 length, quint8 type, int depth) {
        if (length * primitiveSize(type) > m_data.size() - m_pos) throw Error();
        if (type == PtByte) {
            need(length);
            obj.bytes = m_data.mid(m_pos, length);
            m_pos += length;
        } else {
            readValues(obj.values, length, nullptr, depth, true, type);
        }
    }

    Value readRecord(int depth) {
        if (depth > kMaxDepth) throw Error();
        const quint8 type = readByte();
        switch (type) {
        case ClassWithId: {
            const qint32 id = readInt32();
            const qint32 metadataId = readInt32();
            auto it = m_classes.constFind(metadataId);
            if (it == m_classes.constEnd()) throw Error();
            const ClassMeta meta = it.value();
            return storeClass(id, meta, depth);
        }
        case SystemClassWithMembers:
        case ClassWithMembers: {
            qint32 id = 0;
            ClassMeta meta = readClassInfo(id);
            if (type == ClassWithMembers) readInt32();   // library id
            return storeClass(id, meta, depth);
        }
        case SystemClassWithMembersAndTypes:
        case ClassWithMembersAndTypes: {
            qint32 id = 0;
            ClassMeta meta = readClassInfo(id);
            readMemberTypeInfo(meta);
            if (type == ClassWithMembersAndTypes) readInt32();   // library id
            return storeClass(id, meta, depth);
        }
        case BinaryObjectString: {
            const qint32 id = readInt32();
            Value v;
            v.kind = Value::String;
            v.s = readString();
            m_strings.insert(id, v.s);
            return v;
        }
        case BinaryArray: {
            const qint32 id = readInt32();
            const quint8 arrayType = readByte();
            const qint32 rank = readCount();
            if (rank < 1 || rank > 32) throw Error();
            qint64 length = 1;
            for (qint32 r = 0; r < rank; ++r) {
                length *= readCount();
                if (length > std::numeric_limits<qint32>::max()) throw Error();
            }
            if (arrayType >= 3 && arrayType <= 5)            // *Offset variants
                for (qint32 r = 0; r < rank; ++r) readInt32();
            const quint8 binaryType = readByte();
            const quint8 info = readAdditionalInfo(binaryType);
            Object obj;
            if (binaryType == BtPrimitive) {
                readPrimitiveArray(obj, length, info, depth);
            } else {
                if (length > kMaxElements) throw Error();
                readValues(obj.values, length, nullptr, depth);
            }
            return storeArray(id, std::move(obj));
        }
        case ArraySinglePrimitive: {
            const qint32 id = readInt32();
            const qint32 length = readCount();
            const quint8 primitive = readByte();
            Object obj;
            readPrimitiveArray(obj, length, primitive, depth);
            return storeArray(id, std::move(obj));
        }
        case ArraySingleObject:
        case ArraySingleString: {
            const qint32 id = readInt32();
            const qint32 length = readCount();
            if (length > kMaxElements) throw Error();
            Object obj;
            readValues(obj.values, length, nullptr, depth);
            return storeArray(id, std::move(obj));
        }
        case MemberPrimitiveTyped:
            return readPrimitive(readByte());
        case MemberReference:
            return refTo(readInt32());
        case ObjectNull:
            return Value();
        case MessageEnd: {
            Value v;
            v.kind = Value::End;
            return v;
        }
        case BinaryLibrary: {
            readInt32();
            readString();
            Value v;
            v.kind = Value::Library;
            return v;
        }
        case ObjectNullMultiple256:
        case ObjectNullMultiple: {
            Value v;
            v.kind = Value::NullRun;
            v.i = type == ObjectNullMultiple256 ? readByte() : readCount();
            return v;
        }
        default:
            throw Error();   // method calls/returns never appear in a document
        }
    }

    const QByteArray &m_data;
    qint64 m_pos;
    qint32 m_rootId = 0;
    QHash<qint32, Object> m_objects;
    QHash<qint32, QString> m_strings;
    QHash<qint32, ClassMeta> m_classes;
    std::vector<qint32> m_order;          // objects in stream order
};

// Members of base classes are prefixed "Base+name"; match either form.
const Value *member(const Object *obj, const QString &name) {
    if (!obj) return nullptr;
    for (int i = 0; i < obj->memberNames.size() && i < static_cast<int>(obj->values.size()); ++i)
        if (obj->memberNames[i] == name) return &obj->values[i];
    const QString suffix = QLatin1Char('+') + name;
    for (int i = 0; i < obj->memberNames.size() && i < static_cast<int>(obj->values.size()); ++i)
        if (obj->memberNames[i].endsWith(suffix)) return &obj->values[i];
    return nullptr;
}

qint64 intOf(const Value *v, qint64 fallback = 0) {
    if (!v) return fallback;
    if (v->kind == Value::Int) return v->i;
    if (v->kind == Value::Double) return static_cast<qint64>(v->d);
    return fallback;
}

// LayerBlendMode order used by Paint.NET 4+ (blendMode.value__).
BlendMode blendFromIndex(qint64 index) {
    static const BlendMode modes[] = {
        BlendMode::Normal, BlendMode::Multiply, BlendMode::Additive, BlendMode::ColorBurn,
        BlendMode::ColorDodge, BlendMode::Reflect, BlendMode::Glow, BlendMode::Overlay,
        BlendMode::Difference, BlendMode::Negation, BlendMode::Lighten, BlendMode::Darken,
        BlendMode::Screen, BlendMode::Xor,
    };
    if (index < 0 || index >= qint64(std::size(modes))) return BlendMode::Normal;
    return modes[index];
}

// Paint.NET 3.x stores a UserBlendOp object, e.g. "PaintDotNet.UserBlendOps+MultiplyBlendOp".
BlendMode blendFromClassName(QString name) {
    name = name.mid(name.lastIndexOf(QLatin1Char('+')) + 1);
    name = name.mid(name.lastIndexOf(QLatin1Char('.')) + 1);
    if (name.endsWith(QLatin1String("BlendOp"))) name.chop(7);
    static const QHash<QString, BlendMode> table = {
        {"Normal", BlendMode::Normal},       {"Multiply", BlendMode::Multiply},
        {"Additive", BlendMode::Additive},   {"ColorBurn", BlendMode::ColorBurn},
        {"ColorDodge", BlendMode::ColorDodge}, {"Reflect", BlendMode::Reflect},
        {"Glow", BlendMode::Glow},           {"Overlay", BlendMode::Overlay},
        {"Difference", BlendMode::Difference}, {"Negation", BlendMode::Negation},
        {"Lighten", BlendMode::Lighten},     {"Darken", BlendMode::Darken},
        {"Screen", BlendMode::Screen},       {"Xor", BlendMode::Xor},
    };
    return table.value(name, BlendMode::Normal);
}

quint32 readUInt32BE(const QByteArray &data, qint64 &pos) {
    if (pos + 4 > data.size()) throw Error();
    const auto *p = reinterpret_cast<const uchar *>(data.constData() + pos);
    pos += 4;
    return (quint32(p[0]) << 24) | (quint32(p[1]) << 16) | (quint32(p[2]) << 8) | quint32(p[3]);
}

// Inflates one gzip member into exactly `outSize` bytes.
bool gunzipInto(const char *src, qint64 srcSize, char *dst, qint64 outSize) {
    z_stream zs{};
    if (inflateInit2(&zs, 16 + MAX_WBITS) != Z_OK) return false;
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(src));
    zs.avail_in = static_cast<uInt>(srcSize);
    zs.next_out = reinterpret_cast<Bytef *>(dst);
    zs.avail_out = static_cast<uInt>(outSize);
    const int rc = inflate(&zs, Z_FINISH);
    const bool ok = rc == Z_STREAM_END && zs.avail_out == 0;
    inflateEnd(&zs);
    return ok;
}

// Reads one deferred MemoryBlock: format byte (0 = gzip, 1 = raw), the chunk
// size, then every chunk as (index, byte count, bytes), possibly out of order.
QByteArray readDeferredBlock(const QByteArray &data, qint64 &pos, qint64 length) {
    if (pos >= data.size()) throw Error();
    const quint8 format = static_cast<quint8>(data[pos++]);
    if (format > 1) throw Error();
    const quint32 chunkSize = readUInt32BE(data, pos);
    if (chunkSize == 0) throw Error();
    const qint64 chunkCount = (length + chunkSize - 1) / chunkSize;

    QByteArray out(length, Qt::Uninitialized);
    std::vector<bool> seen(static_cast<size_t>(chunkCount), false);
    for (qint64 c = 0; c < chunkCount; ++c) {
        const quint32 number = readUInt32BE(data, pos);
        const quint32 size = readUInt32BE(data, pos);
        if (number >= chunkCount || seen[number] || pos + size > data.size()) throw Error();
        seen[number] = true;
        const qint64 offset = qint64(number) * chunkSize;
        const qint64 expected = qMin<qint64>(chunkSize, length - offset);
        if (format == 0) {
            if (!gunzipInto(data.constData() + pos, size, out.data() + offset, expected))
                throw Error();
        } else {
            if (size != expected) throw Error();
            std::memcpy(out.data() + offset, data.constData() + pos, size);
        }
        pos += size;
    }
    return out;
}

QImage surfaceToImage(const QByteArray &pixels, int width, int height, int stride) {
    if (width <= 0 || height <= 0 || stride <= 0) return {};
    const int bytesPerPixel = stride / width;
    if ((bytesPerPixel != 4 && bytesPerPixel != 3)
        || qint64(stride) * height > pixels.size()) return {};
    QImage image(width, height, QImage::Format_ARGB32);
    if (image.isNull()) return {};
    for (int y = 0; y < height; ++y) {
        const auto *src = reinterpret_cast<const uchar *>(pixels.constData()) + qint64(y) * stride;
        auto *dst = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < width; ++x, src += bytesPerPixel) {
            // Paint.NET's ColorBgra: B, G, R, A in memory, straight alpha.
            const int a = bytesPerPixel == 4 ? src[3] : 255;
            dst[x] = qRgba(src[2], src[1], src[0], a);
        }
    }
    return image;
}

} // namespace

bool isPdnFile(const QString &filePath) {
    QFile file(filePath);
    return file.open(QIODevice::ReadOnly) && file.read(4) == "PDN3";
}

bool read(const QString &filePath, ImageData &out, QString *error) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) *error = file.errorString();
        return false;
    }
    return read(file.readAll(), out, error);
}

bool read(const QByteArray &data, ImageData &out, QString *error) {
    auto fail = [error](const QString &message) {
        if (error) *error = message;
        return false;
    };
    if (data.size() < 9 || !data.startsWith("PDN3"))
        return fail(QStringLiteral("not a Paint.NET image"));

    const auto *p = reinterpret_cast<const uchar *>(data.constData());
    const qint64 headerSize = p[4] | (p[5] << 8) | (p[6] << 16);
    qint64 pos = 7 + headerSize;
    if (pos + 2 > data.size() || p[pos] != 0x00 || p[pos + 1] != 0x01)
        return fail(QStringLiteral("unsupported Paint.NET file layout"));
    pos += 2;

    try {
        NrbfReader nrbf(data, pos);
        nrbf.parse();
        pos = nrbf.position();

        const auto &objects = nrbf.objects();
        auto it = objects.constFind(nrbf.rootId());
        if (it == objects.constEnd()) return fail(QStringLiteral("missing document"));
        const Object *document = &it.value();

        ImageData image;
        image.width = static_cast<int>(intOf(member(document, "width")));
        image.height = static_cast<int>(intOf(member(document, "height")));
        if (image.width <= 0 || image.height <= 0 || image.width > 65535 || image.height > 65535)
            return fail(QStringLiteral("invalid image size"));

        // Deferred pixel blocks follow the NRBF stream in serialization order.
        QHash<const Object *, QByteArray> blockData;
        for (qint32 id : nrbf.order()) {
            const Object &obj = objects.constFind(id).value();
            if (!obj.className.endsWith(QLatin1String("MemoryBlock"))) continue;
            const qint64 length = intOf(member(&obj, "length64"), intOf(member(&obj, "length"), -1));
            if (length < 0 || length > (qint64(1) << 31)) return fail(QStringLiteral("invalid layer data"));
            if (intOf(member(&obj, "deferred"))) {
                blockData.insert(&obj, readDeferredBlock(data, pos, length));
            } else if (const Object *bytes = nrbf.deref(member(&obj, "pointerData"))) {
                blockData.insert(&obj, bytes->bytes.left(length));
            }
        }

        // Document.layers is a LayerList (an ArrayList: _items padded, _size real).
        const Object *layerList = nrbf.deref(member(document, "layers"));
        const Object *items = nrbf.deref(member(layerList, "_items"));
        if (!items) {
            layerList = nrbf.deref(member(layerList, "items"));
            items = nrbf.deref(member(layerList, "_items"));
        }
        if (!items) return fail(QStringLiteral("missing layer list"));
        const qint64 count = qMin<qint64>(intOf(member(layerList, "_size"), qint64(items->values.size())),
                                          qint64(items->values.size()));

        for (qint64 i = 0; i < count; ++i) {
            const Object *layer = nrbf.deref(&items->values[i]);
            if (!layer) return fail(QStringLiteral("invalid layer"));
            const Object *props = nrbf.deref(member(layer, "Layer+properties"));
            const Object *bitmapProps = nrbf.deref(member(layer, "properties"));
            const Object *surface = nrbf.deref(member(layer, "surface"));
            const Object *scan0 = nrbf.deref(member(surface, "scan0"));
            if (!surface || !scan0 || !blockData.contains(scan0))
                return fail(QStringLiteral("missing layer pixels"));

            LayerData out;
            out.image = surfaceToImage(blockData.value(scan0),
                                       static_cast<int>(intOf(member(surface, "width"))),
                                       static_cast<int>(intOf(member(surface, "height"))),
                                       static_cast<int>(intOf(member(surface, "stride"))));
            if (out.image.isNull()) return fail(QStringLiteral("invalid layer pixels"));
            out.name = nrbf.stringOf(member(props, "name"));
            out.visible = intOf(member(props, "visible"), 1) != 0;
            out.opacity = qBound(0, static_cast<int>(intOf(member(props, "opacity"), 255)), 255) / 255.0f;
            if (const Object *mode = nrbf.deref(member(props, "blendMode")))
                out.blendMode = blendFromIndex(intOf(member(mode, "value__")));
            else if (const Object *op = nrbf.deref(member(bitmapProps, "blendOp")))
                out.blendMode = blendFromClassName(op->className);
            image.layers.push_back(std::move(out));
        }
        if (image.layers.empty()) return fail(QStringLiteral("no layers"));
        out = std::move(image);
        return true;
    } catch (const Error &) {
        return fail(QStringLiteral("corrupted or unsupported Paint.NET file"));
    } catch (const std::bad_alloc &) {
        return fail(QStringLiteral("image too large"));
    }
}

} // namespace Pdn
