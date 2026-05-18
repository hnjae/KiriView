// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ARCHIVEFORMAT_H
#define KIRIVIEW_ARCHIVEFORMAT_H

#include <QString>
#include <QStringList>
#include <QUrl>
#include <optional>

namespace KiriView {
enum class ArchiveStorageBackend {
    None,
    KZip,
    KTar,
    K7Zip,
    LibArchive,
};

enum class ArchiveOpenMatchKind {
    ComicBook,
    GeneralArchive,
};

struct ArchiveOpenMatch {
    QString scheme;
    ArchiveOpenMatchKind kind = ArchiveOpenMatchKind::GeneralArchive;
};

bool isSupportedArchiveRootScheme(const QString &scheme);
ArchiveStorageBackend archiveStorageBackendForRootScheme(const QString &scheme);
bool archiveRootSchemeUsesKioFuse(const QString &scheme);
QStringList supportedComicBookArchiveExtensions();
QStringList supportedComicBookArchiveMimeTypes();
bool isComicBookArchiveFileName(const QString &name);
bool isComicBookArchiveUrl(const QUrl &url);
std::optional<ArchiveOpenMatch> comicBookArchiveMatchForFileName(const QString &fileName);
std::optional<ArchiveOpenMatch> directArchiveOpenMatchForFileName(const QString &fileName);
std::optional<ArchiveOpenMatch> comicBookArchiveMatchForMimeTypeName(const QString &mimeTypeName);
std::optional<ArchiveOpenMatch> directArchiveOpenMatchForMimeTypeName(const QString &mimeTypeName);
std::optional<ArchiveOpenMatch> directArchiveOpenMatchForUrl(const QUrl &url);
QString comicBookArchiveKioSchemeForFileName(const QString &fileName);
QString directArchiveOpenKioSchemeForFileName(const QString &fileName);
QString comicBookArchiveKioSchemeForMimeTypeName(const QString &mimeTypeName);
QString directArchiveOpenKioSchemeForMimeTypeName(const QString &mimeTypeName);
QString comicBookArchiveKioSchemeForUrl(const QUrl &url);
QString directArchiveOpenKioSchemeForUrl(const QUrl &url);
QString comicBookArchiveMarkerForRootScheme(const QString &scheme);
QStringList directArchiveOpenMarkersForRootScheme(const QString &scheme);
}

#endif
