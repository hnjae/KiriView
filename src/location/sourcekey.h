// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SOURCEKEY_H
#define KIRIVIEW_SOURCEKEY_H

#include <QHash>
#include <QString>
#include <QUrl>
#include <QtGlobal>

namespace KiriView {
struct SourceKey {
    QUrl normalizedUrl;
    QString identity;
    bool valid = false;
};

struct OrdinaryFileSourceKey {
    SourceKey file;
};

struct DirectMediaSourceKey {
    SourceKey media;
};

struct DirectMediaScopeKey {
    DirectMediaSourceKey current;
    SourceKey parent;
    quint64 generation = 0;
};

struct ImageDocumentPageSourceKey {
    SourceKey scope;
    SourceKey page;
    QString pageKind;
};

struct OpenedCollectionEntrySourceKey {
    SourceKey scope;
    SourceKey entry;
    QString collectionKind;
};

struct ThumbnailSourceKey {
    int rowNumber = 0;
    QUrl url;
    QString label;
    QString pageKind;
    QString sourceKind;
    QString rowIdentity;
    quint64 navigationGeneration = 0;
};

struct PredecodeCandidateKey {
    SourceKey payload;
    SourceKey scope;
    quint64 generation = 0;
};

struct RenderSurfaceKey {
    QString surfaceIdentity;
    quint64 surfaceGeneration = 0;
    quint64 renderContextGeneration = 0;
    QString pageRole;
    QString renderSourceFamily;
};

SourceKey sourceKeyForUrl(const QUrl &url);
SourceKey sourceKeyForDirectMediaCurrentUrl(const QUrl &url);
SourceKey sourceKeyForDirectMediaParentUrl(const QUrl &url);
OrdinaryFileSourceKey ordinaryFileSourceKeyForUrl(const QUrl &url);
DirectMediaSourceKey directMediaSourceKeyForUrl(const QUrl &url);
DirectMediaScopeKey directMediaScopeKeyForUrls(
    const QUrl &currentUrl, const QUrl &parentUrl, quint64 generation);
ImageDocumentPageSourceKey imageDocumentPageSourceKey(
    const QUrl &scopeUrl, const QUrl &pageUrl, const QString &pageKind);
OpenedCollectionEntrySourceKey openedCollectionEntrySourceKey(
    const QUrl &scopeUrl, const QUrl &entryUrl, const QString &collectionKind);
ThumbnailSourceKey thumbnailSourceKey(int rowNumber, const QUrl &url, const QString &label,
    const QString &pageKind, const QString &sourceKind, quint64 navigationGeneration);
PredecodeCandidateKey predecodeCandidateKey(
    const QUrl &payloadUrl, const QUrl &scopeUrl, quint64 generation);
RenderSurfaceKey renderSurfaceKey(const QString &surfaceIdentity, quint64 surfaceGeneration,
    quint64 renderContextGeneration, const QString &pageRole, const QString &renderSourceFamily);
bool sameSourceKey(const SourceKey &left, const SourceKey &right);
bool sameOrdinaryFileSourceKey(
    const OrdinaryFileSourceKey &left, const OrdinaryFileSourceKey &right);
bool sameDirectMediaSourceKey(const DirectMediaSourceKey &left, const DirectMediaSourceKey &right);
bool sameDirectMediaScopeKey(const DirectMediaScopeKey &left, const DirectMediaScopeKey &right);
bool sameImageDocumentPageSourceKey(
    const ImageDocumentPageSourceKey &left, const ImageDocumentPageSourceKey &right);
bool sameOpenedCollectionEntrySourceKey(
    const OpenedCollectionEntrySourceKey &left, const OpenedCollectionEntrySourceKey &right);
bool sameThumbnailSourceKey(const ThumbnailSourceKey &left, const ThumbnailSourceKey &right);
bool samePredecodeCandidateKey(
    const PredecodeCandidateKey &left, const PredecodeCandidateKey &right);
bool sameRenderSurfaceKey(const RenderSurfaceKey &left, const RenderSurfaceKey &right);
bool sameSourceUrlKey(const QUrl &left, const QUrl &right);
bool sameSourceUrlKeyOrEmpty(const QUrl &left, const QUrl &right);
uint qHash(const SourceKey &key, uint seed = 0);
uint qHash(const OrdinaryFileSourceKey &key, uint seed = 0);
uint qHash(const DirectMediaSourceKey &key, uint seed = 0);
uint qHash(const DirectMediaScopeKey &key, uint seed = 0);
uint qHash(const ImageDocumentPageSourceKey &key, uint seed = 0);
uint qHash(const OpenedCollectionEntrySourceKey &key, uint seed = 0);
uint qHash(const ThumbnailSourceKey &key, uint seed = 0);
uint qHash(const PredecodeCandidateKey &key, uint seed = 0);
uint qHash(const RenderSurfaceKey &key, uint seed = 0);
}

#endif
