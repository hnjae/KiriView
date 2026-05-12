// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEBYTECOST_H
#define KIRIVIEW_IMAGEBYTECOST_H

#include <QImage>
#include <QSize>
#include <QtGlobal>

namespace KiriView {
qsizetype imageByteCost(const QImage &image);
qsizetype estimatedRgbaByteCost(const QSize &size);
}

#endif
