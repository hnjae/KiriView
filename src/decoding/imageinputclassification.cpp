// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageinputclassification.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/imageinputclassification.cxx.h"

namespace {
KiriView::ImageInputKind imageInputKindFromRust(KiriView::RustImageInputKind kind)
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

KiriView::QtRasterFormat qtRasterFormatFromRust(KiriView::RustQtRasterFormat format)
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

KiriView::ImageDecodeDataSource imageDecodeDataSourceFromRust(
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
ImageInputClassification classifyImageInput(const QByteArray &data, const QString &fileName)
{
    const QByteArray utf8FileName = fileName.toUtf8();
    const RustImageInputClassification classification
        = rustClassifyImageInput(Bridge::rustBytes(data), Bridge::rustStr(utf8FileName));
    return ImageInputClassification {
        imageInputKindFromRust(classification.kind),
        qtRasterFormatFromRust(classification.qt_format),
        imageDecodeDataSourceFromRust(classification.data_source),
    };
}

QByteArray qtImageReaderFormat(QtRasterFormat format)
{
    switch (format) {
    case QtRasterFormat::Png:
        return QByteArrayLiteral("png");
    case QtRasterFormat::Jpeg:
        return QByteArrayLiteral("jpeg");
    case QtRasterFormat::Gif:
        return QByteArrayLiteral("gif");
    case QtRasterFormat::Webp:
        return QByteArrayLiteral("webp");
    case QtRasterFormat::Bmp:
        return QByteArrayLiteral("bmp");
    case QtRasterFormat::Tiff:
        return QByteArrayLiteral("tiff");
    case QtRasterFormat::Jxl:
        return QByteArrayLiteral("jxl");
    case QtRasterFormat::Jp2:
        return QByteArrayLiteral("jp2");
    case QtRasterFormat::None:
        return {};
    }
    return {};
}
}
