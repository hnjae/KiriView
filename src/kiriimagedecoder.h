// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_KIRIIMAGEDECODER_H
#define KIRIVIEW_KIRIIMAGEDECODER_H

#include "apngdecoder.h"
#include "imagebytecost.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QSizeF>
#include <QString>
#include <QtGlobal>
#include <memory>
#include <optional>
#include <variant>
#include <vector>

namespace KiriView {
inline constexpr int fallbackTextureSizeMax = 16384;

struct DecodedImageFailure {
    QString errorString;
};

struct StaticDecodedImage {
    QImage image;
};

struct SvgDecodedImage {
    QByteArray data;
    QSize svgIntrinsicSize;
};

struct DecodedAnimationImage {
    std::vector<AnimationFrame> frames;
    int loopCount = 0;
};

struct ReaderAnimationImage {
    QImage firstFrame;
    QByteArray data;
    QByteArray format;
    int loopCount = 0;
    int firstFrameDelay = 0;
};

struct HeifSequenceAnimationImage {
    QImage firstFrame;
    QByteArray data;
    int firstFrameDelay = 0;
};

using DecodedImageResult = std::variant<DecodedImageFailure, StaticDecodedImage, SvgDecodedImage,
    DecodedAnimationImage, ReaderAnimationImage, HeifSequenceAnimationImage>;

enum class HeifSequenceOpenStatus {
    NotHeif,
    NotSequence,
    Success,
    Error,
};

struct HeifSequenceOpenResult {
    HeifSequenceOpenStatus status = HeifSequenceOpenStatus::NotHeif;
    QString errorString;
};

class HeifSequenceReader final
{
public:
    HeifSequenceReader();
    ~HeifSequenceReader();

    HeifSequenceReader(const HeifSequenceReader &) = delete;
    HeifSequenceReader &operator=(const HeifSequenceReader &) = delete;
    HeifSequenceReader(HeifSequenceReader &&) noexcept;
    HeifSequenceReader &operator=(HeifSequenceReader &&) noexcept;

    HeifSequenceOpenResult open(QByteArray data);
    std::optional<AnimationFrame> readNextFrame(QString *errorString);
    void close();

private:
    class Private;
    std::unique_ptr<Private> d;
};

QImage displayReadyImage(const QImage &image);
QSize svgRasterSize(const QSizeF &displaySize, qreal devicePixelRatio, int maximumTextureSize);
QImage renderSvgImage(const QByteArray &data, const QSize &size);
DecodedImageResult decodeImageData(const QByteArray &data);
bool decodedImageResultIsPredecodeCacheable(const DecodedImageResult &result, qsizetype byteBudget);
}

#endif
