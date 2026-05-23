// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageinputclassificationconversion.h"

namespace {
KiriView::ImageInputKind imageInputKindFromBridge(KiriView::RustImageInputKind kind)
{
    switch (kind) {
    case KiriView::RustImageInputKind::Unknown:
        return KiriView::ImageInputKind::Unknown;
    case KiriView::RustImageInputKind::Svg:
        return KiriView::ImageInputKind::Svg;
    case KiriView::RustImageInputKind::Apng:
        return KiriView::ImageInputKind::Apng;
    case KiriView::RustImageInputKind::HeifFamily:
        return KiriView::ImageInputKind::HeifFamily;
    case KiriView::RustImageInputKind::Raw:
        return KiriView::ImageInputKind::Raw;
    case KiriView::RustImageInputKind::QtRaster:
        return KiriView::ImageInputKind::QtRaster;
    }

    return KiriView::ImageInputKind::Unknown;
}

KiriView::QtRasterFormat qtRasterFormatFromBridge(KiriView::RustQtRasterFormat format)
{
    switch (format) {
    case KiriView::RustQtRasterFormat::None:
        return KiriView::QtRasterFormat::None;
    case KiriView::RustQtRasterFormat::Png:
        return KiriView::QtRasterFormat::Png;
    case KiriView::RustQtRasterFormat::Jpeg:
        return KiriView::QtRasterFormat::Jpeg;
    case KiriView::RustQtRasterFormat::Gif:
        return KiriView::QtRasterFormat::Gif;
    case KiriView::RustQtRasterFormat::Webp:
        return KiriView::QtRasterFormat::Webp;
    case KiriView::RustQtRasterFormat::Bmp:
        return KiriView::QtRasterFormat::Bmp;
    case KiriView::RustQtRasterFormat::Tiff:
        return KiriView::QtRasterFormat::Tiff;
    case KiriView::RustQtRasterFormat::Jxl:
        return KiriView::QtRasterFormat::Jxl;
    case KiriView::RustQtRasterFormat::Jp2:
        return KiriView::QtRasterFormat::Jp2;
    }

    return KiriView::QtRasterFormat::None;
}

KiriView::ImageDecodeDataSource imageDecodeDataSourceFromBridge(
    KiriView::RustImageDecodeDataSource dataSource)
{
    switch (dataSource) {
    case KiriView::RustImageDecodeDataSource::Original:
        return KiriView::ImageDecodeDataSource::Original;
    case KiriView::RustImageDecodeDataSource::AvifCompatible:
        return KiriView::ImageDecodeDataSource::AvifCompatible;
    }

    return KiriView::ImageDecodeDataSource::Original;
}
}

namespace KiriView {
ImageInputClassification imageInputClassificationFromBridge(
    const RustImageInputClassification &classification)
{
    return ImageInputClassification {
        imageInputKindFromBridge(classification.kind),
        qtRasterFormatFromBridge(classification.qt_format),
        imageDecodeDataSourceFromBridge(classification.data_source),
    };
}
}
