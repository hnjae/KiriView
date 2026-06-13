// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_RAWTHUMBNAILPREVIEW_H
#define KIRIVIEW_RAWTHUMBNAILPREVIEW_H

#include "decoding/imagedecoderequest.h"
#include "rendering/staticimage.h"

#include <QByteArray>
#include <QImage>
#include <QSize>
#include <QString>
#include <functional>
#include <optional>

namespace kiriview {
enum class RawEmbeddedThumbnailPreviewStatus {
    Ready,
    Missing,
    Invalid,
    Failed,
};

struct RawEmbeddedThumbnailPreviewResult {
    RawEmbeddedThumbnailPreviewStatus status = RawEmbeddedThumbnailPreviewStatus::Missing;
    QImage image;
    QSize originalSize;
    QString errorString;
};

using RawEmbeddedThumbnailPreviewExtractor = std::function<RawEmbeddedThumbnailPreviewResult(
    const QByteArray &, const ImageDecodeRequest &)>;

std::optional<QSize> rawEmbeddedThumbnailPreviewTrustedOriginalSize(
    const QByteArray &data, const ImageDecodeRequest &request);
RawEmbeddedThumbnailPreviewResult rawEmbeddedThumbnailPreviewResult(
    const QByteArray &data, const ImageDecodeRequest &request);
std::optional<StaticDisplayImagePayload> rawEmbeddedThumbnailPreviewDisplayPayload(
    const ImageDecodeRequest &request, RawEmbeddedThumbnailPreviewResult result);
}

#endif
