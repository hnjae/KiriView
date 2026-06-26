// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "sourcekey.h"

#include "location/imageurl.h"

#include <QDir>

namespace {
bool isRelativeLocalPathUrl(const QUrl& url)
{
    return url.scheme().isEmpty() && url.isRelative() && !url.path().isEmpty();
}

QUrl absoluteLocalFileIdentityUrl(const QUrl& url)
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

QUrl normalizedSourceIdentityUrl(const QUrl& url)
{
    const QUrl normalizedUrl = url.adjusted(QUrl::NormalizePathSegments);
    if (normalizedUrl.isLocalFile() || isRelativeLocalPathUrl(normalizedUrl)) {
        return absoluteLocalFileIdentityUrl(normalizedUrl);
    }

    return normalizedUrl;
}
}

namespace kiriview {
SourceKey sourceKeyForUrl(const QUrl& url)
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

SourceKey sourceKeyForDirectMediaCurrentUrl(const QUrl& url)
{
    return sourceKeyForUrl(navigationSourceUrl(url));
}

SourceKey sourceKeyForDirectMediaParentUrl(const QUrl& url)
{
    return sourceKeyForUrl(navigationSourceUrl(url));
}

OrdinaryFileSourceKey ordinaryFileSourceKeyForUrl(const QUrl& url)
{
    return OrdinaryFileSourceKey { sourceKeyForUrl(url) };
}

DirectMediaSourceKey directMediaSourceKeyForUrl(const QUrl& url)
{
    return DirectMediaSourceKey { sourceKeyForDirectMediaCurrentUrl(url) };
}

DirectMediaScopeKey directMediaScopeKeyForUrls(
    const QUrl& currentUrl, const QUrl& parentUrl, quint64 generation)
{
    return DirectMediaScopeKey {
        directMediaSourceKeyForUrl(currentUrl),
        sourceKeyForDirectMediaParentUrl(parentUrl),
        generation,
    };
}

ImageDocumentPageSourceKey imageDocumentPageSourceKey(
    const QUrl& scopeUrl, const QUrl& pageUrl, const QString& pageKind)
{
    return ImageDocumentPageSourceKey {
        sourceKeyForUrl(scopeUrl),
        sourceKeyForUrl(pageUrl),
        pageKind,
    };
}

OpenedCollectionEntrySourceKey openedCollectionEntrySourceKey(
    const QUrl& scopeUrl, const QUrl& entryUrl, const QString& collectionKind)
{
    return OpenedCollectionEntrySourceKey {
        sourceKeyForUrl(scopeUrl),
        sourceKeyForUrl(entryUrl),
        collectionKind,
    };
}

ThumbnailSourceKey thumbnailSourceKey(int rowNumber, const QUrl& url, const QString& label,
    const QString& pageKind, const QString& sourceKind, quint64 navigationGeneration)
{
    const SourceKey urlKey = sourceKeyForUrl(url);
    const QString rowIdentity = QStringLiteral("%1|%2|%3|%4|%5")
                                    .arg(rowNumber)
                                    .arg(sourceKind, urlKey.identity, label, pageKind);
    return ThumbnailSourceKey {
        rowNumber,
        url,
        label,
        pageKind,
        sourceKind,
        rowIdentity,
        navigationGeneration,
    };
}

PredecodeCandidateKey predecodeCandidateKey(
    const QUrl& payloadUrl, const QUrl& scopeUrl, quint64 generation)
{
    return PredecodeCandidateKey {
        sourceKeyForUrl(payloadUrl),
        sourceKeyForUrl(scopeUrl),
        generation,
    };
}

RenderSurfaceKey renderSurfaceKey(const QString& surfaceIdentity, quint64 surfaceGeneration,
    quint64 renderContextGeneration, const QString& pageRole, const QString& renderSourceFamily)
{
    return RenderSurfaceKey {
        surfaceIdentity,
        surfaceGeneration,
        renderContextGeneration,
        pageRole,
        renderSourceFamily,
    };
}

bool sameSourceKey(const SourceKey& left, const SourceKey& right)
{
    return left.valid && right.valid && left.identity == right.identity;
}

bool sameOrdinaryFileSourceKey(
    const OrdinaryFileSourceKey& left, const OrdinaryFileSourceKey& right)
{
    return sameSourceKey(left.file, right.file);
}

bool sameDirectMediaSourceKey(const DirectMediaSourceKey& left, const DirectMediaSourceKey& right)
{
    return sameSourceKey(left.media, right.media);
}

bool sameDirectMediaScopeKey(const DirectMediaScopeKey& left, const DirectMediaScopeKey& right)
{
    return sameDirectMediaSourceKey(left.current, right.current)
        && sameSourceKey(left.parent, right.parent);
}

bool sameImageDocumentPageSourceKey(
    const ImageDocumentPageSourceKey& left, const ImageDocumentPageSourceKey& right)
{
    return sameSourceKey(left.scope, right.scope) && sameSourceKey(left.page, right.page)
        && left.pageKind == right.pageKind;
}

bool sameOpenedCollectionEntrySourceKey(
    const OpenedCollectionEntrySourceKey& left, const OpenedCollectionEntrySourceKey& right)
{
    return sameSourceKey(left.scope, right.scope) && sameSourceKey(left.entry, right.entry)
        && left.collectionKind == right.collectionKind;
}

bool sameThumbnailSourceKey(const ThumbnailSourceKey& left, const ThumbnailSourceKey& right)
{
    return left.rowIdentity == right.rowIdentity;
}

bool samePredecodeCandidateKey(
    const PredecodeCandidateKey& left, const PredecodeCandidateKey& right)
{
    return sameSourceKey(left.payload, right.payload) && sameSourceKey(left.scope, right.scope);
}

bool sameRenderSurfaceKey(const RenderSurfaceKey& left, const RenderSurfaceKey& right)
{
    return left.surfaceIdentity == right.surfaceIdentity
        && left.surfaceGeneration == right.surfaceGeneration
        && left.renderContextGeneration == right.renderContextGeneration
        && left.pageRole == right.pageRole && left.renderSourceFamily == right.renderSourceFamily;
}

bool sameSourceUrlKey(const QUrl& left, const QUrl& right)
{
    return sameSourceKey(sourceKeyForUrl(left), sourceKeyForUrl(right));
}

bool sameSourceUrlKeyOrEmpty(const QUrl& left, const QUrl& right)
{
    if (left.isEmpty() || right.isEmpty()) {
        return left.isEmpty() && right.isEmpty();
    }

    return sameSourceUrlKey(left, right);
}

uint qHash(const SourceKey& key, uint seed) { return qHash(key.identity, seed); }

uint qHash(const OrdinaryFileSourceKey& key, uint seed) { return qHash(key.file, seed); }

uint qHash(const DirectMediaSourceKey& key, uint seed) { return qHash(key.media, seed); }

uint qHash(const DirectMediaScopeKey& key, uint seed)
{
    return qHashMulti(seed, key.current, key.parent);
}

uint qHash(const ImageDocumentPageSourceKey& key, uint seed)
{
    return qHashMulti(seed, key.scope, key.page, key.pageKind);
}

uint qHash(const OpenedCollectionEntrySourceKey& key, uint seed)
{
    return qHashMulti(seed, key.scope, key.entry, key.collectionKind);
}

uint qHash(const ThumbnailSourceKey& key, uint seed) { return qHash(key.rowIdentity, seed); }

uint qHash(const PredecodeCandidateKey& key, uint seed)
{
    return qHashMulti(seed, key.payload, key.scope);
}

uint qHash(const RenderSurfaceKey& key, uint seed)
{
    return qHashMulti(seed, key.surfaceIdentity, key.surfaceGeneration, key.renderContextGeneration,
        key.pageRole, key.renderSourceFamily);
}
}
