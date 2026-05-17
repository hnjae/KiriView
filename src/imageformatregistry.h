// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEFORMATREGISTRY_H
#define KIRIVIEW_IMAGEFORMATREGISTRY_H

#include "archiveformat.h"

#include <QString>
#include <QStringList>
#include <QUrl>
#include <optional>

namespace KiriView {
QStringList supportedImageExtensions();
QStringList supportedImageMimeTypes();
QStringList supportedOpenExtensions();
bool isSupportedImageFileName(const QString &name);
bool isSupportedRawImageFileName(const QString &name);
bool isComicBookArchiveFileName(const QString &name);
bool isComicBookArchiveUrl(const QUrl &url);
QString comicBookArchiveKioSchemeForUrl(const QUrl &url);
std::optional<ArchiveOpenMatch> directArchiveOpenMatchForUrl(const QUrl &url);
QString directArchiveOpenKioSchemeForUrl(const QUrl &url);
QStringList openDialogNameFilters();
}

#endif
