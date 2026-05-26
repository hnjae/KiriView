// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEPATH_H
#define KIRIVIEW_ARCHIVEPATH_H

#include <QString>
#include <QUrl>
#include <optional>

namespace KiriView {
class ImagePageScopeLocation;

struct KioFuseArchivePath {
    QString scheme;
    QString path;
};

QString normalizedArchiveRootPath(const QUrl &archiveRootUrl);
QString normalizedArchiveEntryPath(const QString &entryPath);
QString archiveRelativePathForUrl(const QUrl &archiveRootUrl, const QUrl &url);
QString archiveEntryPathForUrl(const ImagePageScopeLocation &archiveDocument, const QUrl &imageUrl);
QUrl archiveEntryUrl(const ImagePageScopeLocation &archiveDocument, const QString &entryPath);
std::optional<KioFuseArchivePath> kioFuseArchivePath(
    const QString &localPath, const QString &runtimeDir);
std::optional<QUrl> kioFuseArchiveUrlForLocalPath(
    const QString &localPath, const QString &runtimeDir);
}

#endif
