// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "apngdecoder.h"

#include <QBuffer>
#include <QIODevice>
#include <QImageReader>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <Qt>
#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

namespace {
constexpr qsizetype pngSignatureSize = 8;
constexpr unsigned char pngSignature[pngSignatureSize]
    = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
constexpr int apngDefaultDelayDenominator = 100;
constexpr unsigned char apngDisposeOpNone = 0;
constexpr unsigned char apngDisposeOpBackground = 1;
constexpr unsigned char apngDisposeOpPrevious = 2;
constexpr unsigned char apngBlendOpSource = 0;
constexpr unsigned char apngBlendOpOver = 1;

struct PngChunk {
    QByteArray type;
    QByteArray data;
};

struct FrameControl {
    quint32 width = 0;
    quint32 height = 0;
    quint32 xOffset = 0;
    quint32 yOffset = 0;
    quint16 delayNum = 0;
    quint16 delayDen = 0;
    unsigned char disposeOp = apngDisposeOpNone;
    unsigned char blendOp = apngBlendOpSource;
};

struct RawFrame {
    FrameControl control;
    QByteArray imageData;
    bool usesDefaultImageData = false;
};

struct ParsedApng {
    quint32 canvasWidth = 0;
    quint32 canvasHeight = 0;
    quint32 playCount = 0;
    QByteArray ihdrData;
    std::vector<PngChunk> headerChunks;
    std::vector<RawFrame> frames;
};

bool hasPngSignature(const QByteArray &data)
{
    if (data.size() < pngSignatureSize) {
        return false;
    }

    for (qsizetype i = 0; i < pngSignatureSize; ++i) {
        if (static_cast<unsigned char>(data.at(i)) != pngSignature[i]) {
            return false;
        }
    }

    return true;
}

bool chunkTypeIs(const QByteArray &type, const char *expected)
{
    return type.size() == 4 && std::equal(type.cbegin(), type.cend(), expected);
}

quint32 readUint32(const QByteArray &data, qsizetype offset)
{
    return (quint32(static_cast<unsigned char>(data.at(offset))) << 24)
        | (quint32(static_cast<unsigned char>(data.at(offset + 1))) << 16)
        | (quint32(static_cast<unsigned char>(data.at(offset + 2))) << 8)
        | quint32(static_cast<unsigned char>(data.at(offset + 3)));
}

quint16 readUint16(const QByteArray &data, qsizetype offset)
{
    return (quint16(static_cast<unsigned char>(data.at(offset))) << 8)
        | quint16(static_cast<unsigned char>(data.at(offset + 1)));
}

void writeUint32(QByteArray *data, qsizetype offset, quint32 value)
{
    (*data)[offset] = static_cast<char>((value >> 24) & 0xff);
    (*data)[offset + 1] = static_cast<char>((value >> 16) & 0xff);
    (*data)[offset + 2] = static_cast<char>((value >> 8) & 0xff);
    (*data)[offset + 3] = static_cast<char>(value & 0xff);
}

void appendUint32(QByteArray *data, quint32 value)
{
    data->append(static_cast<char>((value >> 24) & 0xff));
    data->append(static_cast<char>((value >> 16) & 0xff));
    data->append(static_cast<char>((value >> 8) & 0xff));
    data->append(static_cast<char>(value & 0xff));
}

quint32 crc32(const QByteArray &type, const QByteArray &payload)
{
    quint32 crc = 0xffffffffU;
    const auto update = [&crc](unsigned char byte) {
        crc ^= byte;
        for (int bit = 0; bit < 8; ++bit) {
            const quint32 mask = -(crc & 1U);
            crc = (crc >> 1) ^ (0xedb88320U & mask);
        }
    };

    for (const char byte : type) {
        update(static_cast<unsigned char>(byte));
    }
    for (const char byte : payload) {
        update(static_cast<unsigned char>(byte));
    }

    return crc ^ 0xffffffffU;
}

void appendPngChunk(QByteArray *png, const QByteArray &type, const QByteArray &payload)
{
    appendUint32(png, static_cast<quint32>(payload.size()));
    png->append(type);
    png->append(payload);
    appendUint32(png, crc32(type, payload));
}

KiriView::ApngDecodeResult notApng() { return {}; }

KiriView::ApngDecodeResult apngError(const QString &message)
{
    KiriView::ApngDecodeResult result;
    result.status = KiriView::ApngDecodeStatus::Error;
    result.errorString = message.isEmpty()
        ? QStringLiteral("Could not decode the selected APNG animation.")
        : message;
    return result;
}

bool finishCurrentFrame(
    std::optional<RawFrame> *currentFrame, std::vector<RawFrame> *frames, QString *errorString)
{
    if (!currentFrame->has_value()) {
        return true;
    }

    if ((*currentFrame)->imageData.isEmpty()) {
        *errorString = QStringLiteral("Could not decode the selected APNG animation.");
        return false;
    }

    frames->push_back(std::move(**currentFrame));
    currentFrame->reset();
    return true;
}

std::optional<FrameControl> readFrameControl(const QByteArray &data, QString *errorString)
{
    if (data.size() != 26) {
        *errorString = QStringLiteral("Could not decode the selected APNG animation.");
        return std::nullopt;
    }

    FrameControl control;
    control.width = readUint32(data, 4);
    control.height = readUint32(data, 8);
    control.xOffset = readUint32(data, 12);
    control.yOffset = readUint32(data, 16);
    control.delayNum = readUint16(data, 20);
    control.delayDen = readUint16(data, 22);
    control.disposeOp = static_cast<unsigned char>(data.at(24));
    control.blendOp = static_cast<unsigned char>(data.at(25));

    if (control.width == 0 || control.height == 0 || control.disposeOp > apngDisposeOpPrevious
        || control.blendOp > apngBlendOpOver) {
        *errorString = QStringLiteral("Could not decode the selected APNG animation.");
        return std::nullopt;
    }

    return control;
}

std::optional<ParsedApng> parseApng(const QByteArray &data, QString *errorString)
{
    if (!hasPngSignature(data)) {
        return std::nullopt;
    }

    qsizetype offset = pngSignatureSize;
    bool seenIhdr = false;
    bool seenActl = false;
    bool seenIdat = false;
    bool seenIend = false;
    quint32 expectedFrameCount = 0;
    ParsedApng parsed;
    std::optional<RawFrame> currentFrame;

    while (offset < data.size()) {
        if (data.size() - offset < 12) {
            *errorString = QStringLiteral("Could not decode the selected PNG image.");
            return std::nullopt;
        }

        const quint32 length = readUint32(data, offset);
        if (length > static_cast<quint32>(std::numeric_limits<int>::max())
            || data.size() - offset - 12 < static_cast<qsizetype>(length)) {
            *errorString = QStringLiteral("Could not decode the selected PNG image.");
            return std::nullopt;
        }

        const QByteArray type = data.mid(offset + 4, 4);
        const QByteArray chunkData = data.mid(offset + 8, length);
        offset += 12 + length;

        if (chunkTypeIs(type, "IHDR")) {
            if (seenIhdr || chunkData.size() != 13) {
                *errorString = QStringLiteral("Could not decode the selected PNG image.");
                return std::nullopt;
            }

            seenIhdr = true;
            parsed.ihdrData = chunkData;
            parsed.canvasWidth = readUint32(chunkData, 0);
            parsed.canvasHeight = readUint32(chunkData, 4);
            if (parsed.canvasWidth == 0 || parsed.canvasHeight == 0
                || parsed.canvasWidth > static_cast<quint32>(std::numeric_limits<int>::max())
                || parsed.canvasHeight > static_cast<quint32>(std::numeric_limits<int>::max())) {
                *errorString = QStringLiteral("Could not decode the selected PNG image.");
                return std::nullopt;
            }
        } else if (chunkTypeIs(type, "acTL")) {
            if (!seenIhdr || seenIdat || chunkData.size() != 8) {
                *errorString = QStringLiteral("Could not decode the selected APNG animation.");
                return std::nullopt;
            }

            seenActl = true;
            expectedFrameCount = readUint32(chunkData, 0);
            parsed.playCount = readUint32(chunkData, 4);
            if (expectedFrameCount == 0) {
                *errorString = QStringLiteral("Could not decode the selected APNG animation.");
                return std::nullopt;
            }
        } else if (chunkTypeIs(type, "fcTL")) {
            if (!seenActl) {
                *errorString = QStringLiteral("Could not decode the selected APNG animation.");
                return std::nullopt;
            }

            if (!finishCurrentFrame(&currentFrame, &parsed.frames, errorString)) {
                return std::nullopt;
            }

            const std::optional<FrameControl> control = readFrameControl(chunkData, errorString);
            if (!control.has_value()) {
                return std::nullopt;
            }

            const quint64 right = quint64(control->xOffset) + control->width;
            const quint64 bottom = quint64(control->yOffset) + control->height;
            if (right > parsed.canvasWidth || bottom > parsed.canvasHeight) {
                *errorString = QStringLiteral("Could not decode the selected APNG animation.");
                return std::nullopt;
            }

            currentFrame = RawFrame { *control, QByteArray(), !seenIdat };
        } else if (chunkTypeIs(type, "IDAT")) {
            if (!seenIhdr) {
                *errorString = QStringLiteral("Could not decode the selected PNG image.");
                return std::nullopt;
            }

            seenIdat = true;
            if (currentFrame.has_value() && currentFrame->usesDefaultImageData) {
                currentFrame->imageData.append(chunkData);
            }
        } else if (chunkTypeIs(type, "fdAT")) {
            if (!currentFrame.has_value() || chunkData.size() < 4) {
                *errorString = QStringLiteral("Could not decode the selected APNG animation.");
                return std::nullopt;
            }

            currentFrame->imageData.append(chunkData.sliced(4));
        } else if (chunkTypeIs(type, "IEND")) {
            if (!finishCurrentFrame(&currentFrame, &parsed.frames, errorString)) {
                return std::nullopt;
            }
            seenIend = true;
            break;
        } else if (!seenIdat) {
            parsed.headerChunks.push_back(PngChunk { type, chunkData });
        }
    }

    if (!seenActl) {
        return std::nullopt;
    }
    if (!seenIend || expectedFrameCount != parsed.frames.size()) {
        *errorString = QStringLiteral("Could not decode the selected APNG animation.");
        return std::nullopt;
    }

    return parsed;
}

int frameDelay(const FrameControl &control)
{
    const quint16 denominator
        = control.delayDen == 0 ? apngDefaultDelayDenominator : control.delayDen;
    const quint64 delay = quint64(control.delayNum) * 1000U / denominator;
    return static_cast<int>(std::min<quint64>(delay, std::numeric_limits<int>::max()));
}

int loopCount(quint32 playCount)
{
    if (playCount == 0) {
        return -1;
    }

    const quint32 repeatsAfterFirstPlay = playCount - 1;
    return static_cast<int>(
        std::min<quint32>(repeatsAfterFirstPlay, std::numeric_limits<int>::max()));
}

QByteArray framePngData(const ParsedApng &apng, const RawFrame &frame)
{
    QByteArray png;
    png.reserve(pngSignatureSize + apng.ihdrData.size() + frame.imageData.size() + 128);
    png.append(reinterpret_cast<const char *>(pngSignature), pngSignatureSize);

    QByteArray ihdr = apng.ihdrData;
    writeUint32(&ihdr, 0, frame.control.width);
    writeUint32(&ihdr, 4, frame.control.height);
    appendPngChunk(&png, QByteArrayLiteral("IHDR"), ihdr);

    for (const PngChunk &chunk : apng.headerChunks) {
        appendPngChunk(&png, chunk.type, chunk.data);
    }

    appendPngChunk(&png, QByteArrayLiteral("IDAT"), frame.imageData);
    appendPngChunk(&png, QByteArrayLiteral("IEND"), QByteArray());
    return png;
}

std::optional<QImage> decodeFrameImage(const QByteArray &pngData, QString *errorString)
{
    QBuffer buffer;
    buffer.setData(pngData);
    if (!buffer.open(QIODevice::ReadOnly)) {
        *errorString = QStringLiteral("Could not read the selected APNG frame data.");
        return std::nullopt;
    }

    QImageReader reader(&buffer, QByteArrayLiteral("png"));
    QImage image = reader.read();
    if (image.isNull()) {
        *errorString = reader.errorString();
        return std::nullopt;
    }

    return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

void clearRect(QImage *image, const QRect &rect)
{
    QPainter painter(image);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.fillRect(rect, Qt::transparent);
}

std::optional<std::vector<KiriView::AnimationFrame>> renderFrames(
    const ParsedApng &apng, QString *errorString)
{
    QImage canvas(static_cast<int>(apng.canvasWidth), static_cast<int>(apng.canvasHeight),
        QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);

    std::vector<KiriView::AnimationFrame> frames;
    frames.reserve(apng.frames.size());

    for (std::size_t index = 0; index < apng.frames.size(); ++index) {
        const RawFrame &frame = apng.frames[index];
        const QByteArray pngData = framePngData(apng, frame);
        const std::optional<QImage> decodedFrame = decodeFrameImage(pngData, errorString);
        if (!decodedFrame.has_value()) {
            return std::nullopt;
        }
        if (decodedFrame->size()
            != QSize(
                static_cast<int>(frame.control.width), static_cast<int>(frame.control.height))) {
            *errorString = QStringLiteral("Could not decode the selected APNG animation.");
            return std::nullopt;
        }

        const QRect frameRect(static_cast<int>(frame.control.xOffset),
            static_cast<int>(frame.control.yOffset), static_cast<int>(frame.control.width),
            static_cast<int>(frame.control.height));
        const std::optional<QImage> previousFrameRegion
            = frame.control.disposeOp == apngDisposeOpPrevious
            ? std::make_optional(canvas.copy(frameRect))
            : std::nullopt;

        {
            QPainter painter(&canvas);
            painter.setCompositionMode(frame.control.blendOp == apngBlendOpSource
                    ? QPainter::CompositionMode_Source
                    : QPainter::CompositionMode_SourceOver);
            painter.drawImage(frameRect.topLeft(), *decodedFrame);
        }

        frames.push_back(KiriView::AnimationFrame { canvas, frameDelay(frame.control) });

        if (frame.control.disposeOp == apngDisposeOpBackground
            || (index == 0 && frame.control.disposeOp == apngDisposeOpPrevious)) {
            clearRect(&canvas, frameRect);
        } else if (previousFrameRegion.has_value()) {
            QPainter painter(&canvas);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.drawImage(frameRect.topLeft(), *previousFrameRegion);
        }
    }

    return frames;
}
}

namespace KiriView {
ApngDecodeResult decodeApngAnimation(const QByteArray &data)
{
    QString errorString;
    const std::optional<ParsedApng> parsed = parseApng(data, &errorString);
    if (!parsed.has_value()) {
        return errorString.isEmpty() ? notApng() : apngError(errorString);
    }

    std::optional<std::vector<AnimationFrame>> frames = renderFrames(*parsed, &errorString);
    if (!frames.has_value()) {
        return apngError(errorString);
    }

    ApngDecodeResult result;
    result.status = ApngDecodeStatus::Success;
    result.animation.frames = std::move(*frames);
    result.animation.loopCount = loopCount(parsed->playCount);
    return result;
}
}
