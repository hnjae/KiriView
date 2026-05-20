// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEINPUTCLASSIFICATION_H
#define KIRIVIEW_IMAGEINPUTCLASSIFICATION_H

#include <QByteArray>
#include <QString>

namespace KiriView {
enum class ImageInputKind {
    Unknown,
    Svg,
    Apng,
    HeifFamily,
    Raw,
    QtRaster,
};

enum class QtRasterFormat {
    None,
    Png,
    Jpeg,
    Gif,
    Webp,
    Bmp,
    Tiff,
    Jxl,
    Jp2,
};

enum class ImageDecodeDataSource {
    Original,
    AvifCompatible,
};

struct ImageInputClassification {
    ImageInputKind kind = ImageInputKind::Unknown;
    QtRasterFormat qtFormat = QtRasterFormat::None;
    ImageDecodeDataSource dataSource = ImageDecodeDataSource::Original;
};

ImageInputClassification classifyImageInput(const QByteArray &data, const QString &fileName);
QByteArray qtImageReaderFormat(QtRasterFormat format);
}

#endif
