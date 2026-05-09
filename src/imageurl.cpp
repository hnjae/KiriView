// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageurl.h"

#include "kiriview/src/imageurl.cxx.h"
#include "rustqtconversion.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QString>
#include <QtGlobal>
#include <optional>
#include <sys/types.h>
#include <sys/xattr.h>

namespace {
constexpr const char *documentPortalHostPathAttribute = "user.document-portal.host-path";

std::optional<QUrl> kioFuseArchiveUrl(const QString &localPath)
{
    const QString runtimeDir = QFile::decodeName(qgetenv("XDG_RUNTIME_DIR"));
    const QByteArray localPathBytes = localPath.toUtf8();
    const QByteArray runtimeDirBytes = runtimeDir.toUtf8();
    const KiriView::RustKioFuseArchivePath archivePath = KiriView::rustKioFuseArchivePath(
        KiriView::Bridge::rustStr(localPathBytes), KiriView::Bridge::rustStr(runtimeDirBytes));
    if (!archivePath.found) {
        return std::nullopt;
    }

    QUrl url;
    url.setScheme(KiriView::Bridge::qtString(archivePath.scheme));
    url.setPath(KiriView::Bridge::qtString(archivePath.path));
    if (!url.isValid() || url.path().isEmpty()) {
        return std::nullopt;
    }

    return url;
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
        return std::nullopt;
    }

    QByteArray value;
    value.resize(valueSize);

    const ssize_t bytesRead = getxattr(encodedLocalPath.constData(),
        documentPortalHostPathAttribute, value.data(), static_cast<std::size_t>(value.size()));
    if (bytesRead <= 0) {
        return std::nullopt;
    }

    value.resize(bytesRead);
    if (value.endsWith('\0')) {
        value.chop(1);
    }

    const QString hostPath = QFile::decodeName(value);
    if (hostPath.isEmpty() || hostPath == localPath) {
        return std::nullopt;
    }

    return navigationUrlForLocalPath(hostPath);
}
}

namespace KiriView {
QUrl normalizedImageUrl(const QUrl &url) { return url.adjusted(QUrl::NormalizePathSegments); }

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
        return hostUrl.value();
    }

    if (url.isLocalFile()) {
        const std::optional<QUrl> kioUrl = kioFuseArchiveUrl(url.toLocalFile());
        if (kioUrl.has_value()) {
            return kioUrl.value();
        }
    }

    return url;
}

bool sameNormalizedUrl(const QUrl &left, const QUrl &right)
{
    return left.matches(right, QUrl::NormalizePathSegments);
}

bool sameContainerNavigationUrl(const QUrl &left, const QUrl &right)
{
    return !left.isEmpty() && !right.isEmpty() && sameNormalizedUrl(left, right);
}
}
