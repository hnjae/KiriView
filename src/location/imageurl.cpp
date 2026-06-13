// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "location/imageurl.h"

#include "archive/archivepath.h"
#include "navigation/navigationlogging.h"

#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QString>
#include <QtGlobal>
#include <cerrno>
#include <optional>
#include <sys/types.h>
#include <sys/xattr.h>

namespace {
constexpr const char *documentPortalHostPathAttribute = "user.document-portal.host-path";

std::optional<QUrl> kioFuseArchiveUrl(const QString &localPath)
{
    const QString runtimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
    return kiriview::kioFuseArchiveUrlForLocalPath(localPath, runtimeDir);
}

QUrl navigationUrlForLocalPath(const QString &localPath)
{
    const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(localPath);
    if (kioUrl.has_value()) {
        return kioUrl.value();
    }

    return QUrl::fromLocalFile(localPath);
}

QUrl normalizedContainerBaseUrl(const QUrl &url)
{
    QUrl normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
    normalizedUrl.setQuery(QString());
    normalizedUrl.setFragment(QString());
    return normalizedUrl;
}

std::optional<QUrl> documentPortalHostUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const QString localPath = url.toLocalFile();
    const QByteArray encodedLocalPath = QFile::encodeName(localPath);
    if (encodedLocalPath.isEmpty()) {
        return std::nullopt;
    }

    // File dialogs can return document-portal URLs; navigation needs the real directory.
    const ssize_t valueSize
        = getxattr(encodedLocalPath.constData(), documentPortalHostPathAttribute, nullptr, 0);
    if (valueSize <= 0) {
        qCDebug(kiriviewNavigationLog) << "document portal host path missing"
                                       << "url" << url << "errno" << errno;
        return std::nullopt;
    }

    QByteArray value;
    value.resize(valueSize);

    const ssize_t bytesRead = getxattr(encodedLocalPath.constData(),
        documentPortalHostPathAttribute, value.data(), static_cast<std::size_t>(value.size()));
    if (bytesRead <= 0) {
        qCDebug(kiriviewNavigationLog) << "document portal host path read failed"
                                       << "url" << url << "errno" << errno;
        return std::nullopt;
    }

    value.resize(bytesRead);
    if (value.endsWith('\0')) {
        value.chop(1);
    }

    const QString hostPath = QFile::decodeName(value);
    if (hostPath.isEmpty() || hostPath == localPath) {
        qCDebug(kiriviewNavigationLog) << "document portal host path ignored"
                                       << "url" << url << "hostPath" << hostPath;
        return std::nullopt;
    }

    const QUrl hostUrl = navigationUrlForLocalPath(hostPath);
    qCDebug(kiriviewNavigationLog)
        << "document portal host path resolved"
        << "url" << url << "hostPath" << hostPath << "hostUrl" << hostUrl;
    return hostUrl;
}
}

namespace kiriview {
bool DirectoryNavigationLocation::isValid() const
{
    return fileUrl.isValid() && !fileUrl.isEmpty() && directoryUrl.isValid()
        && !directoryUrl.isEmpty();
}

QUrl normalizedUrlForIdentity(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments); }

QString normalizedUrlIdentityKey(const QUrl &url, QUrl::ComponentFormattingOptions options)
{
    return normalizedUrlForIdentity(url).toString(options);
}

std::optional<QUrl> normalizedValidUrlForIdentity(const QUrl &url)
{
    const QUrl normalizedUrl = normalizedUrlForIdentity(url);
    if (!normalizedUrl.isValid() || normalizedUrl.isEmpty()) {
        return std::nullopt;
    }

    return normalizedUrl;
}

QUrl normalizedImageUrl(const QUrl &url) { return normalizedUrlForIdentity(url); }

std::optional<QUrl> normalizedValidImageUrl(const QUrl &url)
{
    return normalizedValidUrlForIdentity(url);
}

QUrl normalizedDirectoryUrlForIdentity(const QUrl &url) { return normalizedUrlForIdentity(url); }

QString directoryUrlIdentityKey(const QUrl &url, QUrl::ComponentFormattingOptions options)
{
    return normalizedUrlIdentityKey(normalizedDirectoryUrlForIdentity(url), options);
}

QUrl normalizedFileContainerUrl(const QUrl &url)
{
    QUrl normalizedUrl = normalizedContainerBaseUrl(url);

    if (normalizedUrl.isLocalFile()) {
        normalizedUrl = QUrl::fromLocalFile(QDir::cleanPath(normalizedUrl.toLocalFile()));
    }
    return normalizedUrl;
}

QUrl normalizedDirectoryContainerUrl(const QUrl &url)
{
    QUrl normalizedUrl = normalizedContainerBaseUrl(url);

    QString path = normalizedUrl.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path += QLatin1Char('/');
        normalizedUrl.setPath(path);
    }
    return normalizedUrl;
}

QUrl parentDirectoryUrlForFileNavigation(const QUrl &url)
{
    return url.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
}

QUrl parentUrlForContainerNavigation(const QUrl &containerUrl)
{
    QUrl parentSourceUrl = containerUrl.adjusted(QUrl::NormalizePathSegments);
    QString path = parentSourceUrl.path();
    if (path.size() > 1 && path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
        parentSourceUrl.setPath(path);
    }

    return parentSourceUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
}

QUrl navigationSourceUrl(const QUrl &url)
{
    const std::optional<QUrl> hostUrl = documentPortalHostUrl(url);
    if (hostUrl.has_value()) {
        qCDebug(kiriviewNavigationLog) << "navigation source url uses document portal host"
                                       << "url" << url << "navigationUrl" << hostUrl.value();
        return hostUrl.value();
    }

    if (url.isLocalFile()) {
        const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(url.toLocalFile());
        if (kioUrl.has_value()) {
            qCDebug(kiriviewNavigationLog) << "navigation source url uses kio-fuse archive"
                                           << "url" << url << "navigationUrl" << kioUrl.value();
            return kioUrl.value();
        }
    }

    return url;
}

DirectoryNavigationLocation directoryNavigationLocationForFileUrl(const QUrl &url)
{
    QUrl fileUrl = navigationSourceUrl(url);
    return DirectoryNavigationLocation {
        fileUrl,
        parentDirectoryUrlForFileNavigation(fileUrl),
    };
}

bool sameNormalizedUrl(const QUrl &left, const QUrl &right)
{
    return left.matches(right, QUrl::NormalizePathSegments);
}

bool sameNormalizedUrlOrEmpty(const QUrl &left, const QUrl &right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return sameNormalizedUrl(left, right);
}

bool sameContainerNavigationUrl(const QUrl &left, const QUrl &right)
{
    return !left.isEmpty() && !right.isEmpty() && sameNormalizedUrl(left, right);
}
}
