// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivepath.h"

#include "imagelocation.h"

#include <QDir>

namespace {
QString archiveRootPathWithTrailingSlash(QString path)
{
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
    }

    return path;
}
}

namespace KiriView {
QString normalizedArchiveRootPath(const QUrl &archiveRootUrl)
{
    return archiveRootPathWithTrailingSlash(QDir::cleanPath(archiveRootUrl.path()));
}

QString normalizedArchiveEntryPath(const QString &entryPath)
{
    QString cleanPath = QDir::cleanPath(entryPath);
    while (cleanPath.startsWith(QStringLiteral("./"))) {
        cleanPath.remove(0, 2);
    }
    if (cleanPath == QStringLiteral(".") || cleanPath == QStringLiteral("..")
        || cleanPath.startsWith(QStringLiteral("../")) || cleanPath.startsWith(QLatin1Char('/'))) {
        return {};
    }

    return cleanPath;
}

QString archiveRelativePathForUrl(const QUrl &archiveRootUrl, const QUrl &url)
{
    if (url.isEmpty() || archiveRootUrl.isEmpty() || url.scheme() != archiveRootUrl.scheme()) {
        return {};
    }

    const QString rootPath = normalizedArchiveRootPath(archiveRootUrl);
    const QString path = QDir::cleanPath(url.path());
    if (path.size() <= rootPath.size() || !path.startsWith(rootPath)) {
        return {};
    }

    return path.mid(rootPath.size());
}

QString archiveEntryPathForUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &imageUrl)
{
    if (archiveDocument.isEmpty()) {
        return {};
    }

    return normalizedArchiveEntryPath(
        archiveRelativePathForUrl(archiveDocument.rootUrl(), imageUrl));
}

QUrl archiveEntryUrl(const ArchiveDocumentLocation &archiveDocument, const QString &entryPath)
{
    const QString cleanEntryPath = normalizedArchiveEntryPath(entryPath);
    if (archiveDocument.isEmpty() || cleanEntryPath.isEmpty()) {
        return {};
    }

    QUrl url = archiveDocument.rootUrl();
    url.setPath(normalizedArchiveRootPath(url) + cleanEntryPath);
    url.setQuery(QString());
    url.setFragment(QString());
    return url;
}
}
