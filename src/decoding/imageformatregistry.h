// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEFORMATREGISTRY_H
#define KIRIVIEW_IMAGEFORMATREGISTRY_H

#include <QString>
#include <QStringList>

namespace KiriView {
QStringList supportedImageExtensions();
QStringList supportedImageMimeTypes();
QStringList supportedOpenExtensions();
bool isSupportedImageFileName(const QString &name);
bool isSupportedRawImageFileName(const QString &name);
QStringList openDialogNameFilters();
}

#endif
