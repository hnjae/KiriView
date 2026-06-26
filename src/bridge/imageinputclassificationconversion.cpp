// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageinputclassificationconversion.h"

namespace {
kiriview::ImageInputKind imageInputKindFromBridge(kiriview::RustImageInputKind kind)
{
    switch (kind) {
    case kiriview::RustImageInputKind::Unknown:
        return kiriview::ImageInputKind::Unknown;
    case kiriview::RustImageInputKind::Svg:
        return kiriview::ImageInputKind::Svg;
    case kiriview::RustImageInputKind::Apng:
        return kiriview::ImageInputKind::Apng;
    case kiriview::RustImageInputKind::HeifFamily:
        return kiriview::ImageInputKind::HeifFamily;
    case kiriview::RustImageInputKind::Raw:
        return kiriview::ImageInputKind::Raw;
    case kiriview::RustImageInputKind::QtRaster:
        return kiriview::ImageInputKind::QtRaster;
    }

    return kiriview::ImageInputKind::Unknown;
}

kiriview::QtRasterFormat qtRasterFormatFromBridge(kiriview::RustQtRasterFormat format)
{
    switch (format) {
    case kiriview::RustQtRasterFormat::None:
        return kiriview::QtRasterFormat::None;
    case kiriview::RustQtRasterFormat::Png:
        return kiriview::QtRasterFormat::Png;
    case kiriview::RustQtRasterFormat::Jpeg:
        return kiriview::QtRasterFormat::Jpeg;
    case kiriview::RustQtRasterFormat::Gif:
        return kiriview::QtRasterFormat::Gif;
    case kiriview::RustQtRasterFormat::Webp:
        return kiriview::QtRasterFormat::Webp;
    case kiriview::RustQtRasterFormat::Bmp:
        return kiriview::QtRasterFormat::Bmp;
    case kiriview::RustQtRasterFormat::Tiff:
        return kiriview::QtRasterFormat::Tiff;
    case kiriview::RustQtRasterFormat::Jxl:
        return kiriview::QtRasterFormat::Jxl;
    case kiriview::RustQtRasterFormat::Jp2:
        return kiriview::QtRasterFormat::Jp2;
    }

    return kiriview::QtRasterFormat::None;
}

kiriview::ImageDecodeDataSource imageDecodeDataSourceFromBridge(
    kiriview::RustImageDecodeDataSource dataSource)
{
    switch (dataSource) {
    case kiriview::RustImageDecodeDataSource::Original:
        return kiriview::ImageDecodeDataSource::Original;
    case kiriview::RustImageDecodeDataSource::AvifCompatible:
        return kiriview::ImageDecodeDataSource::AvifCompatible;
    }

    return kiriview::ImageDecodeDataSource::Original;
}
}

namespace kiriview {
ImageInputClassification imageInputClassificationFromBridge(
    const RustImageInputClassification& classification)
{
    return ImageInputClassification {
        imageInputKindFromBridge(classification.kind),
        qtRasterFormatFromBridge(classification.qt_format),
        imageDecodeDataSourceFromBridge(classification.data_source),
    };
}
}
