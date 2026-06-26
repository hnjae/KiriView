// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageinputclassification.h"

#include "bridge/imageinputclassificationconversion.h"
#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/imageinputclassification.cxx.h"

namespace kiriview {
ImageInputClassification classifyImageInput(const QByteArray& data, const QString& fileName)
{
    const QByteArray utf8FileName = fileName.toUtf8();
    const RustImageInputClassification classification
        = rustClassifyImageInput(Bridge::rustBytes(data), Bridge::rustStr(utf8FileName));
    return imageInputClassificationFromBridge(classification);
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
