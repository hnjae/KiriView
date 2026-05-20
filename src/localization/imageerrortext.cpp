// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "localization/imageerrortext.h"

#include <KLocalizedString>

namespace KiriView {
QString imageErrorText(ImageErrorTextId id)
{
    switch (id) {
    case ImageErrorTextId::ReadImageData:
        return i18n("Could not read the selected image data.");
    case ImageErrorTextId::DecodePngImage:
        return i18n("Could not decode the selected PNG image.");
    case ImageErrorTextId::DecodeApngAnimation:
        return i18n("Could not decode the selected APNG animation.");
    case ImageErrorTextId::DecodeImageAnimation:
        return i18n("Could not decode the selected image animation.");
    case ImageErrorTextId::EmptyArchive:
        return i18n("The selected archive does not contain any supported images.");
    case ImageErrorTextId::OpenArchive:
        return i18n("Could not open the selected archive.");
    case ImageErrorTextId::OpenComicBookArchive:
        return i18n("Could not open the selected comic book archive.");
    case ImageErrorTextId::DeleteFile:
        return i18n("Could not delete the selected file.");
    case ImageErrorTextId::DetermineSvgImageSize:
        return i18n("Could not determine the selected SVG image size.");
    case ImageErrorTextId::RenderSvgTile:
        return i18n("Could not render the selected SVG tile.");
    case ImageErrorTextId::RenderSvgImage:
        return i18n("Could not render the selected SVG image.");
    case ImageErrorTextId::DetermineJpegFirstDisplaySize:
        return i18n("Could not determine the selected JPEG first-display size.");
    case ImageErrorTextId::ImageFullDecodeFallbackTooLarge:
        return i18n("The selected image is too large for fallback full-image decoding.");
    case ImageErrorTextId::RenderTile:
        return i18n("Could not render the selected tile.");
    case ImageErrorTextId::AllocateTile:
        return i18n("Could not allocate the selected tile.");
    case ImageErrorTextId::DecodeHeifSequence:
        return i18n("Could not decode the selected HEIF image sequence.");
    case ImageErrorTextId::HeifSequenceTrackMissing:
        return i18n("Could not decode the selected HEIF image: sequence track is missing.");
    case ImageErrorTextId::HeifDecodeOptionsAllocationFailed:
        return i18n("Could not decode the selected HEIF image: libheif could not allocate "
                    "decoding options.");
    case ImageErrorTextId::HeifContextAllocationFailed:
        return i18n("Could not decode the selected HEIF image: libheif could not allocate a "
                    "context.");
    case ImageErrorTextId::HeifDecodedImageInvalid:
        return i18n("Could not decode the selected HEIF image: decoded image is invalid.");
    case ImageErrorTextId::HeifDecodedImageSizeInvalid:
        return i18n("Could not decode the selected HEIF image: decoded image size is invalid.");
    case ImageErrorTextId::HeifDecodedPixelDataInvalid:
        return i18n("Could not decode the selected HEIF image: decoded pixel data is invalid.");
    case ImageErrorTextId::HeifDecodedImageAllocationFailed:
        return i18n("Could not decode the selected HEIF image: decoded image allocation failed.");
    case ImageErrorTextId::HeifImageSizeInvalid:
        return i18n("Could not decode the selected HEIF image: image size is invalid.");
    case ImageErrorTextId::HeifFullDecodeFallbackTooLarge:
        return i18n("The selected HEIF image is too large for fallback full-image decoding.");
    case ImageErrorTextId::RawDecodedImageSizeInvalid:
        return i18n("Could not decode the selected RAW image: decoded image size is invalid.");
    case ImageErrorTextId::RawFullDecodeTooLarge:
        return i18n("The selected RAW image is too large for full-image decoding.");
    case ImageErrorTextId::RawDecodedImageInvalid:
        return i18n("Could not decode the selected RAW image: decoded image is invalid.");
    case ImageErrorTextId::RawDecodedPixelFormatUnsupported:
        return i18n("Could not decode the selected RAW image: decoded pixel format is "
                    "unsupported.");
    case ImageErrorTextId::RawDecodedPixelDataInvalid:
        return i18n("Could not decode the selected RAW image: decoded pixel data is invalid.");
    case ImageErrorTextId::RawDecodedImageAllocationFailed:
        return i18n("Could not decode the selected RAW image: decoded image allocation failed.");
    case ImageErrorTextId::RenderRawTile:
        return i18n("Could not render the selected RAW tile.");
    case ImageErrorTextId::UnknownLibheifError:
        return i18n("Unknown libheif error.");
    case ImageErrorTextId::UnknownLibrawError:
        return i18n("Unknown LibRaw error.");
    }

    return {};
}

QString imageErrorActionText(ImageErrorActionTextId id)
{
    switch (id) {
    case ImageErrorActionTextId::InitializeLibheif:
        return i18n("initializing libheif");
    case ImageErrorActionTextId::ReadHeifContainer:
        return i18n("reading the HEIF container");
    case ImageErrorActionTextId::ReadPrimaryImage:
        return i18n("reading the primary image");
    case ImageErrorActionTextId::DecodePrimaryImage:
        return i18n("decoding the primary image");
    case ImageErrorActionTextId::DecodeHeifGridTile:
        return i18n("decoding a HEIF grid tile");
    case ImageErrorActionTextId::DecodeHeifSequence:
        return i18n("decoding the HEIF image sequence");
    case ImageErrorActionTextId::ReadRawImage:
        return i18n("reading the RAW image");
    case ImageErrorActionTextId::UnpackRawImage:
        return i18n("unpacking the RAW image");
    case ImageErrorActionTextId::ProcessRawImage:
        return i18n("processing the RAW image");
    case ImageErrorActionTextId::CreateDisplayImage:
        return i18n("creating the display image");
    }

    return {};
}

QString heifDecodeErrorText(const QString &action, const QString &detail)
{
    return ki18nc("@info:status", "Could not decode the selected HEIF image: %1: %2")
        .subs(action)
        .subs(detail)
        .toString();
}

QString rawDecodeErrorText(const QString &action, const QString &detail)
{
    return ki18nc("@info:status", "Could not decode the selected RAW image: %1: %2")
        .subs(action)
        .subs(detail)
        .toString();
}
}
