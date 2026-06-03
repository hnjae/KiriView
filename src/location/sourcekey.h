// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_SOURCEKEY_H
#define KIRIVIEW_SOURCEKEY_H

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
    QUrl entryUrl;
    QString collectionKind;
};

struct ThumbnailSourceKey {
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
};

SourceKey sourceKeyForUrl(const QUrl &url);
SourceKey sourceKeyForDirectMediaCurrentUrl(const QUrl &url);
SourceKey sourceKeyForDirectMediaParentUrl(const QUrl &url);
OrdinaryFileSourceKey ordinaryFileSourceKeyForUrl(const QUrl &url);
DirectMediaSourceKey directMediaSourceKeyForUrl(const QUrl &url);
DirectMediaScopeKey directMediaScopeKeyForUrls(
    const QUrl &currentUrl, const QUrl &parentUrl, quint64 generation);
bool sameSourceKey(const SourceKey &left, const SourceKey &right);
bool sameOrdinaryFileSourceKey(
    const OrdinaryFileSourceKey &left, const OrdinaryFileSourceKey &right);
bool sameDirectMediaSourceKey(const DirectMediaSourceKey &left, const DirectMediaSourceKey &right);
bool sameDirectMediaScopeKey(const DirectMediaScopeKey &left, const DirectMediaScopeKey &right);
bool sameSourceUrlKey(const QUrl &left, const QUrl &right);
bool sameSourceUrlKeyOrEmpty(const QUrl &left, const QUrl &right);
}

#endif
