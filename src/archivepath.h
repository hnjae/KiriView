// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEPATH_H
#define KIRIVIEW_ARCHIVEPATH_H

#include <QString>
#include <QUrl>

namespace KiriView {
class ArchiveDocumentLocation;

QString normalizedArchiveRootPath(const QUrl &archiveRootUrl);
QString normalizedArchiveEntryPath(const QString &entryPath);
QString archiveRelativePathForUrl(const QUrl &archiveRootUrl, const QUrl &url);
QString archiveEntryPathForUrl(
    const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl);
QUrl archiveEntryUrl(const ArchiveDocumentLocation &archiveDocument, const QString &entryPath);
}

#endif
