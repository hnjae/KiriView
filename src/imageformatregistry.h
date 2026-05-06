// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEFORMATREGISTRY_H
#define KIRIVIEW_IMAGEFORMATREGISTRY_H

#include <QString>
#include <QStringList>
#include <QUrl>

namespace KiriView {
QStringList supportedImageExtensions();
QStringList supportedOpenExtensions();
bool isSupportedImageFileName(const QString &name);
bool isComicBookArchiveFileName(const QString &name);
bool isComicBookArchiveUrl(const QUrl &url);
QString comicBookArchiveKioSchemeForUrl(const QUrl &url);
QString directArchiveOpenKioSchemeForUrl(const QUrl &url);
QStringList openDialogNameFilters();
}

#endif
