// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_HEIFTILESOURCE_H
#define KIRIVIEW_HEIFTILESOURCE_H

#include "staticimage.h"

#include <QByteArray>
#include <QString>
#include <memory>

namespace KiriView {
std::shared_ptr<ImageTileSource> openHeifTileSource(const QByteArray &data, QString *errorString);
}

#endif
