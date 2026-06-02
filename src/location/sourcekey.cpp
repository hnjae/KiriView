// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "sourcekey.h"

#include "location/imageurl.h"

#include <QDir>

namespace {
bool isRelativeLocalPathUrl(const QUrl &url)
{
    return url.scheme().isEmpty() && url.isRelative() && !url.path().isEmpty();
}

QUrl absoluteLocalFileIdentityUrl(const QUrl &url)
{
    QString localPath = url.isLocalFile() ? url.toLocalFile() : url.path();
    localPath = QDir::cleanPath(localPath);
    if (QDir::isRelativePath(localPath)) {
        localPath = QDir::cleanPath(QDir::current().absoluteFilePath(localPath));
    }

    QUrl normalizedUrl = QUrl::fromLocalFile(localPath);
    normalizedUrl.setQuery(url.query());
    normalizedUrl.setFragment(url.fragment());
    return normalizedUrl;
}

QUrl normalizedSourceIdentityUrl(const QUrl &url)
{
    const QUrl normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
    if (normalizedUrl.isLocalFile() || isRelativeLocalPathUrl(normalizedUrl)) {
        return absoluteLocalFileIdentityUrl(normalizedUrl);
    }

    return normalizedUrl;
}
}

namespace KiriView {
SourceKey sourceKeyForUrl(const QUrl &url)
{
    if (url.isEmpty() || !url.isValid()) {
        return {};
    }

    const QUrl normalizedUrl = normalizedSourceIdentityUrl(url);
    const QString identity = normalizedUrl.toString(QUrl::FullyEncoded);
    if (normalizedUrl.isEmpty() || !normalizedUrl.isValid() || identity.isEmpty()) {
        return {};
    }

    return SourceKey {
        normalizedUrl,
        identity,
        true,
    };
}

SourceKey sourceKeyForDirectMediaCurrentUrl(const QUrl &url)
{
    return sourceKeyForUrl(navigationSourceUrl(url));
}

SourceKey sourceKeyForDirectMediaParentUrl(const QUrl &url)
{
    return sourceKeyForUrl(navigationSourceUrl(url));
}

bool sameSourceKey(const SourceKey &left, const SourceKey &right)
{
    return left.valid && right.valid && left.identity == right.identity;
}

bool sameSourceUrlKey(const QUrl &left, const QUrl &right)
{
    return sameSourceKey(sourceKeyForUrl(left), sourceKeyForUrl(right));
}

bool sameSourceUrlKeyOrEmpty(const QUrl &left, const QUrl &right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return sameSourceUrlKey(left, right);
}
}
