// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainer.h"

#include "archiveformat.h"
#include "imageformatregistry.h"
#include "imagenavigationmodel.h"
#include "imageurl.h"
#include "kiriview/src/imagecontainer.cxx.h"

#include <QByteArray>
#include <cstddef>
#include <optional>

namespace {
struct ArchiveDocumentRoot {
    QUrl rootUrl;
    KiriView::ArchiveDocumentKind kind = KiriView::ArchiveDocumentKind::General;
};

struct UrlParts {
    QByteArray scheme;
    QByteArray path;
    bool empty = true;
};

rust::Str rustStringView(const QByteArray &bytes)
{
    return rust::Str(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

QString rustStringToQString(const rust::String &value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

UrlParts urlParts(const QUrl &url)
{
    return UrlParts { url.scheme().toUtf8(), url.path().toUtf8(), url.isEmpty() };
}

std::optional<QUrl> archiveRootUrlForLocalArchive(const QUrl &url, const QString &archiveScheme)
{
    const QByteArray archiveSchemeBytes = archiveScheme.toUtf8();
    const QByteArray localPathBytes = url.toLocalFile().toUtf8();
    const KiriView::RustArchiveRootPath rootPath = KiriView::rustArchiveRootPathForLocalArchive(
        url.isLocalFile(), rustStringView(archiveSchemeBytes), rustStringView(localPathBytes));
    if (!rootPath.found) {
        return std::nullopt;
    }

    QUrl archiveRootUrl;
    archiveRootUrl.setScheme(archiveScheme);
    archiveRootUrl.setPath(rustStringToQString(rootPath.path));
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

KiriView::ArchiveDocumentKind archiveDocumentKindForMatch(const KiriView::ArchiveOpenMatch &match)
{
    switch (match.kind) {
    case KiriView::ArchiveOpenMatchKind::ComicBook:
        return KiriView::ArchiveDocumentKind::ComicBook;
    case KiriView::ArchiveOpenMatchKind::GeneralArchive:
        return KiriView::ArchiveDocumentKind::General;
    }

    return KiriView::ArchiveDocumentKind::General;
}

std::optional<ArchiveDocumentRoot> archiveDocumentRootForLocalArchive(const QUrl &url)
{
    const std::optional<KiriView::ArchiveOpenMatch> match
        = KiriView::directArchiveOpenMatchForUrl(url);
    if (!match.has_value()) {
        return std::nullopt;
    }

    const std::optional<QUrl> rootUrl = archiveRootUrlForLocalArchive(url, match->scheme);
    if (!rootUrl.has_value()) {
        return std::nullopt;
    }

    return ArchiveDocumentRoot { *rootUrl, archiveDocumentKindForMatch(*match) };
}

std::optional<QUrl> containingArchiveRootUrl(
    const QUrl &url, KiriView::RustArchiveRootPath (*rustFunction)(rust::Str, rust::Str))
{
    const QByteArray scheme = url.scheme().toUtf8();
    const QByteArray path = url.path().toUtf8();
    const KiriView::RustArchiveRootPath rootPath
        = rustFunction(rustStringView(scheme), rustStringView(path));
    if (!rootPath.found) {
        return std::nullopt;
    }

    QUrl archiveRootUrl = url;
    archiveRootUrl.setPath(rustStringToQString(rootPath.path));
    archiveRootUrl.setQuery(QString());
    archiveRootUrl.setFragment(QString());
    if (!archiveRootUrl.isValid() || archiveRootUrl.path().isEmpty()) {
        return std::nullopt;
    }

    return archiveRootUrl;
}

QUrl archiveDocumentFileNavigationUrl(const KiriView::DisplayedImageLocation &location)
{
    return KiriView::normalizedFileContainerUrl(
        KiriView::navigationSourceUrl(location.archiveDocumentFileUrl()));
}

bool archiveDocumentContainsUrlInRust(
    const KiriView::ArchiveDocumentLocation &archiveDocument, const QUrl &url)
{
    const UrlParts root = urlParts(archiveDocument.rootUrl());
    const UrlParts candidate = urlParts(url);
    return KiriView::rustArchiveDocumentContainsUrl(archiveDocument.isEmpty(), root.empty,
        rustStringView(root.scheme), rustStringView(root.path), candidate.empty,
        rustStringView(candidate.scheme), rustStringView(candidate.path));
}

}

namespace KiriView {
std::optional<QUrl> comicBookArchiveRootUrl(const QUrl &url)
{
    const QString archiveScheme = KiriView::comicBookArchiveKioSchemeForUrl(url);
    return archiveRootUrlForLocalArchive(url, archiveScheme);
}

std::optional<QUrl> directArchiveOpenRootUrl(const QUrl &url)
{
    const std::optional<ArchiveDocumentRoot> root = archiveDocumentRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return root->rootUrl;
}

std::optional<ArchiveDocumentLocation> archiveDocumentLocationForLocalArchiveUrl(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return std::nullopt;
    }

    const std::optional<ArchiveDocumentRoot> root = archiveDocumentRootForLocalArchive(url);
    if (!root.has_value()) {
        return std::nullopt;
    }

    return ArchiveDocumentLocation::fromUrls(
        normalizedFileContainerUrl(url), root->rootUrl, root->kind);
}

bool isUrlInsideArchiveRoot(const QUrl &url, const QUrl &archiveRootUrl)
{
    const UrlParts root = urlParts(archiveRootUrl);
    const UrlParts candidate = urlParts(url);
    return rustArchiveDocumentContainsUrl(false, root.empty, rustStringView(root.scheme),
        rustStringView(root.path), candidate.empty, rustStringView(candidate.scheme),
        rustStringView(candidate.path));
}

bool archiveDocumentContainsUrl(const ArchiveDocumentLocation &archiveDocument, const QUrl &url)
{
    return archiveDocumentContainsUrlInRust(archiveDocument, url);
}

bool displayedLocationIsInsideArchiveDocument(const DisplayedImageLocation &location)
{
    return archiveDocumentContainsUrl(location.archiveDocument(), location.imageUrl());
}

std::optional<QUrl> containingComicBookArchiveRootUrl(const QUrl &url)
{
    return containingArchiveRootUrl(url, rustContainingComicBookArchiveRootPath);
}

std::optional<QUrl> containingDirectArchiveOpenRootUrl(const QUrl &url)
{
    return containingArchiveRootUrl(url, rustContainingDirectArchiveOpenRootPath);
}

QString windowTitleFileNameForDisplayedLocation(const DisplayedImageLocation &location)
{
    const QByteArray imageFileName = location.imageUrl().fileName().toUtf8();
    const QByteArray archiveFileName = location.archiveDocumentFileUrl().fileName().toUtf8();
    return rustStringToQString(rustWindowTitleFileNameForDisplayedLocation(
        location.imageUrl().isEmpty(), rustStringView(imageFileName),
        displayedLocationIsInsideArchiveDocument(location), rustStringView(archiveFileName)));
}

std::vector<ContainerNavigationCandidate> containerNavigationCandidates(const KFileItemList &items)
{
    std::vector<ContainerNavigationCandidate> candidates;
    candidates.reserve(static_cast<std::size_t>(items.size()));

    for (const KFileItem &item : items) {
        const QString name = item.name();
        if (item.isFile() && item.url().isLocalFile()
            && KiriView::isComicBookArchiveFileName(name)) {
            candidates.push_back(
                ContainerNavigationCandidate { normalizedFileContainerUrl(item.url()), name,
                    ContainerNavigationCandidateType::ComicBookArchive });
        }
    }

    sortContainerNavigationCandidates(&candidates);
    return candidates;
}

QUrl zoomScopeUrlForLocation(const DisplayedImageLocation &location)
{
    if (rustZoomScopeUsesArchiveDocumentFileUrl(
            displayedLocationIsInsideArchiveDocument(location))) {
        return archiveDocumentFileNavigationUrl(location);
    }

    return {};
}

QUrl containerNavigationUrlForLocation(const DisplayedImageLocation &location)
{
    if (!rustContainerNavigationUsesArchiveDocumentFileUrl(location.archiveDocument().isComicBook(),
            displayedLocationIsInsideArchiveDocument(location))) {
        return {};
    }

    return archiveDocumentFileNavigationUrl(location);
}

bool shouldResetRightToLeftReadingForLoad(bool rightToLeftReadingEnabled,
    const ArchiveDocumentLocation &displayedArchiveDocument, const QUrl &sourceUrl,
    const QUrl &containerNavigationUrl)
{
    return rustShouldResetRightToLeftReadingForLoad(rightToLeftReadingEnabled,
        containerNavigationUrl.isEmpty(), displayedArchiveDocument.isComicBook(),
        archiveDocumentContainsUrlInRust(displayedArchiveDocument, sourceUrl));
}
}
