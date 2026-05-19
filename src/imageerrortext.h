// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEERRORTEXT_H
#define KIRIVIEW_IMAGEERRORTEXT_H

#include <QString>

namespace KiriView {
enum class ImageErrorTextId {
    ReadImageData,
    DecodePngImage,
    DecodeApngAnimation,
    DecodeImageAnimation,
    EmptyArchive,
    OpenArchive,
    OpenComicBookArchive,
    DeleteFile,
    DetermineSvgImageSize,
    RenderSvgTile,
    RenderSvgImage,
    DetermineJpegFirstDisplaySize,
    ImageFullDecodeFallbackTooLarge,
    RenderTile,
    AllocateTile,
    DecodeHeifSequence,
    HeifSequenceTrackMissing,
    HeifDecodeOptionsAllocationFailed,
    HeifContextAllocationFailed,
    HeifDecodedImageInvalid,
    HeifDecodedImageSizeInvalid,
    HeifDecodedPixelDataInvalid,
    HeifDecodedImageAllocationFailed,
    HeifImageSizeInvalid,
    HeifFullDecodeFallbackTooLarge,
    RawDecodedImageSizeInvalid,
    RawFullDecodeTooLarge,
    RawDecodedImageInvalid,
    RawDecodedPixelFormatUnsupported,
    RawDecodedPixelDataInvalid,
    RawDecodedImageAllocationFailed,
    RenderRawTile,
    UnknownLibheifError,
    UnknownLibrawError,
};

enum class ImageErrorActionTextId {
    InitializeLibheif,
    ReadHeifContainer,
    ReadPrimaryImage,
    DecodePrimaryImage,
    DecodeHeifGridTile,
    DecodeHeifSequence,
    ReadRawImage,
    UnpackRawImage,
    ProcessRawImage,
    CreateDisplayImage,
};

QString imageErrorText(ImageErrorTextId id);
QString imageErrorActionText(ImageErrorActionTextId id);
QString heifDecodeErrorText(const QString &action, const QString &detail);
QString rawDecodeErrorText(const QString &action, const QString &detail);
}

#endif
