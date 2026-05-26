// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "archivepath.h"

#include "bridge/rustqtconversion.h"
#include "kiriview/src/policy/archivepath.cxx.h"
#include "location/imagelocation.h"

#include <QByteArray>
#include <optional>

namespace {
QString rustStringForQString(const QString &value, rust::String (*rustFunction)(rust::Str))
{
    return KiriView::Bridge::qtString(KiriView::Bridge::rustResultForQString(value, rustFunction));
}
}

namespace KiriView {
QString normalizedArchiveRootPath(const QUrl &archiveRootUrl)
{
    return rustStringForQString(archiveRootUrl.path(), rustNormalizedArchiveRootPath);
}

QString normalizedArchiveEntryPath(const QString &entryPath)
{
    return rustStringForQString(entryPath, rustNormalizedArchiveEntryPath);
}

QString archiveRelativePathForUrl(const QUrl &archiveRootUrl, const QUrl &url)
{
    const QByteArray archiveRootScheme = archiveRootUrl.scheme().toUtf8();
    const QByteArray archiveRootPath = archiveRootUrl.path().toUtf8();
    const QByteArray urlScheme = url.scheme().toUtf8();
    const QByteArray urlPath = url.path().toUtf8();
    return Bridge::qtString(rustArchiveRelativePathForUrl(archiveRootUrl.isEmpty(),
        Bridge::rustStr(archiveRootScheme), Bridge::rustStr(archiveRootPath), url.isEmpty(),
        Bridge::rustStr(urlScheme), Bridge::rustStr(urlPath)));
}

QString archiveEntryPathForUrl(const ImagePageScopeLocation &archiveDocument, const QUrl &imageUrl)
{
    if (archiveDocument.isEmpty()) {
        return {};
    }

    return normalizedArchiveEntryPath(
        archiveRelativePathForUrl(archiveDocument.rootUrl(), imageUrl));
}

QUrl archiveEntryUrl(const ImagePageScopeLocation &archiveDocument, const QString &entryPath)
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

std::optional<KioFuseArchivePath> kioFuseArchivePath(
    const QString &localPath, const QString &runtimeDir)
{
    const QByteArray localPathBytes = localPath.toUtf8();
    const QByteArray runtimeDirBytes = runtimeDir.toUtf8();
    const RustKioFuseArchivePath archivePath
        = rustKioFuseArchivePath(Bridge::rustStr(localPathBytes), Bridge::rustStr(runtimeDirBytes));
    if (!archivePath.found) {
        return std::nullopt;
    }

    return KioFuseArchivePath {
        Bridge::qtString(archivePath.scheme),
        Bridge::qtString(archivePath.path),
    };
}

std::optional<QUrl> kioFuseArchiveUrlForLocalPath(
    const QString &localPath, const QString &runtimeDir)
{
    const std::optional<KioFuseArchivePath> archivePath = kioFuseArchivePath(localPath, runtimeDir);
    if (!archivePath.has_value()) {
        return std::nullopt;
    }

    QUrl url;
    url.setScheme(archivePath->scheme);
    url.setPath(archivePath->path);
    if (!url.isValid() || url.path().isEmpty()) {
        return std::nullopt;
    }

    return url;
}
}
