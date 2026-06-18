// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_QIMAGEREADERTILESOURCE_H
#define KIRIVIEW_QIMAGEREADERTILESOURCE_H

#include "qimagereaderscaledlevelcache.h"
#include "staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QRect>
#include <QSize>
#include <QString>
#include <memory>
#include <optional>
#include <vector>

namespace kiriview {
enum class QImageReaderTileDecodeAttemptKind {
    ReaderClip,
    SourceClip,
    ScaledLevel,
    FullImageFallback,
};

enum class QImageReaderTileDecodeFailureSeverity {
    Error,
};

struct QImageReaderTileDecodeAttemptFailure {
    QImageReaderTileDecodeAttemptKind kind = QImageReaderTileDecodeAttemptKind::ReaderClip;
    QString errorString;
    QImageReaderTileDecodeFailureSeverity severity = QImageReaderTileDecodeFailureSeverity::Error;
    bool retryable = false;
};

struct QImageReaderTileDecodeDiagnostics {
    std::vector<QImageReaderTileDecodeAttemptFailure> failures;

    QString userMessage() const;
};

struct QImageReaderTileDecodeResult {
    std::optional<DecodedTile> tile;
    QImageReaderTileDecodeDiagnostics diagnostics;
};

enum class QImageReaderDisplayDecodeOperation {
    FirstDisplayImage,
    RasterDisplayImage,
    BlockingDisplayImage,
};

enum class QImageReaderDisplayDecodeFailureSeverity {
    Error,
};

struct QImageReaderDisplayDecodeFailure {
    QImageReaderDisplayDecodeOperation operation
        = QImageReaderDisplayDecodeOperation::RasterDisplayImage;
    QString userMessage;
    QString diagnosticDetail;
    QImageReaderDisplayDecodeFailureSeverity severity
        = QImageReaderDisplayDecodeFailureSeverity::Error;
    bool retryable = false;
};

struct QImageReaderDisplayDecodeDiagnostics {
    std::vector<QImageReaderDisplayDecodeFailure> failures;

    QString userMessage() const;
};

struct QImageReaderDisplayDecodeResult {
    QImage image;
    QImageReaderDisplayDecodeDiagnostics diagnostics;
};

struct QImageReaderFirstDisplayDecodeResult {
    FirstDisplayImageDecodeResult firstDisplay;
    QImageReaderDisplayDecodeDiagnostics diagnostics;
};

class QImageReaderTileSource final : public ImageTileSource
{
public:
    static std::shared_ptr<QImageReaderTileSource> open(
        const QByteArray &data, const QByteArray &format, QString *errorString);

    QImageReaderTileSource(
        QByteArray data, QByteArray format, QSize imageSize, StaticImageReaderTransform transform);

    QSize imageSize() const override;
    std::optional<DecodedTile> decodeTile(
        const TileRequest &request, QString *errorString) const override;
    QImageReaderTileDecodeResult decodeTileWithDiagnostics(const TileRequest &request) const;
    QImageReaderFirstDisplayDecodeResult decodeFirstDisplayImageWithDiagnostics(
        const ImageFirstDisplayDecodeContext &context) const;
    FirstDisplayImageDecodeResult decodeFirstDisplayImage(
        const ImageFirstDisplayDecodeContext &context, QString *errorString) const override;
    bool supportsRasterDisplayRefinement() const override;
    QImageReaderDisplayDecodeResult decodeRasterDisplayImageWithDiagnostics(
        const QSize &rasterSize) const;
    QImage decodeRasterDisplayImage(const QSize &rasterSize, QString *errorString) const override;
    QImageReaderDisplayDecodeResult decodeBlockingDisplayImageWithDiagnostics(
        int maximumLongEdge) const;
    QImage decodeBlockingDisplayImage(int maximumLongEdge, QString *errorString) const override;
    qsizetype byteCost() const override;
    StaticImageReaderTransform imageReaderTransform() const override;

private:
    bool supportsJpegScaledFirstDisplay() const;
    std::optional<DecodedTile> decodeReaderClipTile(
        const TileRequest &request, QImageReaderTileDecodeDiagnostics *diagnostics) const;
    std::optional<DecodedTile> decodeCachedOrScaledLevelTile(
        const TileRequest &request, QImageReaderTileDecodeDiagnostics *diagnostics) const;
    std::optional<DecodedTile> decodeFullImageFallbackTile(
        const TileRequest &request, QImageReaderTileDecodeDiagnostics *diagnostics) const;
    QImageReaderDisplayDecodeResult readScaledDisplayImage(
        const QSize &scaledSize, QImageReaderDisplayDecodeOperation operation) const;
    QImage readScaledImage(const QSize &scaledSize, QString *errorString) const;
    QImage readFullImage(QString *errorString) const;
    QImage readSourceClip(const QRect &sourceRect, QString *errorString) const;

    QByteArray m_data;
    QByteArray m_format;
    QSize m_imageSize;
    StaticImageReaderTransform m_transform;
    mutable QImageReaderScaledLevelCache m_scaledLevelCache;
};
}

#endif
