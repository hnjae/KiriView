// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageformatregistry.h"

#include "archiveformat.h"
#include "kiriview/src/imageformatregistry.cxx.h"
#include "rustqtconversion.h"

#include <KLocalizedString>
#include <QMimeDatabase>
#include <QMimeType>
#include <optional>

namespace {
using ArchiveMatchResolver = std::optional<KiriView::ArchiveOpenMatch> (*)(const QString &);

QString schemeForArchiveMatch(const std::optional<KiriView::ArchiveOpenMatch> &match)
{
    return match.has_value() ? match->scheme : QString();
}

std::optional<KiriView::ArchiveOpenMatch> archiveMatchForUrl(const QUrl &url,
    QMimeDatabase::MatchMode mimeMatchMode, ArchiveMatchResolver matchForFileName,
    ArchiveMatchResolver matchForMimeTypeName)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    std::optional<KiriView::ArchiveOpenMatch> extensionMatch = matchForFileName(url.fileName());
    if (extensionMatch.has_value()) {
        return extensionMatch;
    }

    const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(url.toLocalFile(), mimeMatchMode);
    return matchForMimeTypeName(mimeType.name());
}

QString archiveKioSchemeForUrl(const QUrl &url, QMimeDatabase::MatchMode mimeMatchMode,
    ArchiveMatchResolver matchForFileName, ArchiveMatchResolver matchForMimeTypeName)
{
    return schemeForArchiveMatch(
        archiveMatchForUrl(url, mimeMatchMode, matchForFileName, matchForMimeTypeName));
}

std::optional<KiriView::ArchiveOpenMatch> directArchiveOpenMatchForLocalUrl(const QUrl &url)
{
    return archiveMatchForUrl(url, QMimeDatabase::MatchDefault,
        KiriView::directArchiveOpenMatchForFileName,
        KiriView::directArchiveOpenMatchForMimeTypeName);
}
}

namespace KiriView {
QStringList supportedImageExtensions()
{
    return Bridge::qtStringList(rustSupportedImageExtensions());
}

QStringList supportedImageMimeTypes()
{
    return Bridge::qtStringList(rustSupportedImageMimeTypes());
}

QStringList supportedOpenExtensions()
{
    return Bridge::qtStringList(rustSupportedOpenExtensions());
}

bool isSupportedImageFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedImageFileName);
}

bool isSupportedRawImageFileName(const QString &name)
{
    return Bridge::rustResultForQString(name, rustIsSupportedRawImageFileName);
}

bool isComicBookArchiveFileName(const QString &name)
{
    return !KiriView::comicBookArchiveKioSchemeForFileName(name).isEmpty();
}

bool isComicBookArchiveUrl(const QUrl &url)
{
    return !comicBookArchiveKioSchemeForUrl(url).isEmpty();
}

QString comicBookArchiveKioSchemeForUrl(const QUrl &url)
{
    return archiveKioSchemeForUrl(url, QMimeDatabase::MatchExtension,
        KiriView::comicBookArchiveMatchForFileName, KiriView::comicBookArchiveMatchForMimeTypeName);
}

std::optional<ArchiveOpenMatch> directArchiveOpenMatchForUrl(const QUrl &url)
{
    return directArchiveOpenMatchForLocalUrl(url);
}

QString directArchiveOpenKioSchemeForUrl(const QUrl &url)
{
    return schemeForArchiveMatch(directArchiveOpenMatchForLocalUrl(url));
}

QStringList openDialogNameFilters()
{
    const QString extensionFilter = QStringLiteral("*.") + supportedOpenExtensions().join(" *.");
    return {
        ki18nc("@item:inlistbox", "Image and comic book files (%1)")
            .subs(extensionFilter)
            .toString(),
        i18nc("@item:inlistbox", "All files (*)"),
    };
}
}
