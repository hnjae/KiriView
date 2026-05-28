// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatelistsource.h"

#include "location/imagedocumentlocation.h"
#include "location/imageurl.h"

#include <optional>
#include <utility>

namespace {
bool sameImageCandidateListSourcePayload(const KiriView::ImageCandidateListSource::Directory &left,
    const KiriView::ImageCandidateListSource::Directory &right)
{
    return KiriView::sameNormalizedUrl(left.directoryUrl, right.directoryUrl);
}

bool sameImageCandidateListSourcePayload(
    const KiriView::ImageCandidateListSource::OpenedCollectionScope &left,
    const KiriView::ImageCandidateListSource::OpenedCollectionScope &right)
{
    return KiriView::sameOpenedCollectionScopeLocation(
        left.openedCollectionScope, right.openedCollectionScope);
}

template <typename Left, typename Right>
bool sameImageCandidateListSourcePayload(const Left &, const Right &)
{
    return false;
}
}

namespace KiriView {
ImageCandidateListSource ImageCandidateListSource::forDirectory(QUrl directoryUrl)
{
    return ImageCandidateListSource { Directory { std::move(directoryUrl) } };
}

ImageCandidateListSource ImageCandidateListSource::forOpenedCollectionScope(
    OpenedCollectionScopeLocation openedCollectionScope)
{
    return ImageCandidateListSource { OpenedCollectionScope { std::move(openedCollectionScope) } };
}

OpenedCollectionScopeLocation ImageCandidateListSource::openedCollectionScope() const
{
    const auto *archiveSource = std::get_if<OpenedCollectionScope>(&m_source);
    return archiveSource == nullptr ? OpenedCollectionScopeLocation::none()
                                    : archiveSource->openedCollectionScope;
}

ImageCandidateListSource::ImageCandidateListSource(Payload source)
    : m_source(std::move(source))
{
}

bool sameImageCandidateListSource(
    const ImageCandidateListSource &left, const ImageCandidateListSource &right)
{
    return left.visit([&right](const auto &leftSource) {
        return right.visit([&leftSource](const auto &rightSource) {
            return sameImageCandidateListSourcePayload(leftSource, rightSource);
        });
    });
}

ImageCandidateListContext ImageCandidateListContext::forDirectory(
    QUrl currentUrl, QUrl directoryUrl)
{
    return ImageCandidateListContext {
        std::move(currentUrl),
        ImageCandidateListSource::forDirectory(std::move(directoryUrl)),
    };
}

ImageCandidateListContext ImageCandidateListContext::forOpenedCollectionScope(
    QUrl currentUrl, OpenedCollectionScopeLocation openedCollectionScope)
{
    return ImageCandidateListContext {
        std::move(currentUrl),
        ImageCandidateListSource::forOpenedCollectionScope(std::move(openedCollectionScope)),
    };
}

ImageCandidateListContext ImageCandidateListContext::forSource(
    QUrl currentUrl, ImageCandidateListSource source)
{
    return ImageCandidateListContext {
        std::move(currentUrl),
        std::move(source),
    };
}

const QUrl &ImageCandidateListContext::currentUrl() const { return m_currentUrl; }

const ImageCandidateListSource &ImageCandidateListContext::source() const { return m_source; }

OpenedCollectionScopeLocation ImageCandidateListContext::openedCollectionScope() const
{
    return m_source.openedCollectionScope();
}

ImageCandidateListContext::ImageCandidateListContext(
    QUrl currentUrl, ImageCandidateListSource source)
    : m_currentUrl(std::move(currentUrl))
    , m_source(std::move(source))
{
}

bool sameImageCandidateListContext(
    const ImageCandidateListContext &left, const ImageCandidateListContext &right)
{
    return sameNormalizedUrl(left.currentUrl(), right.currentUrl())
        && sameImageCandidateListSource(left.source(), right.source());
}

std::optional<ImageCandidateListContext> imageCandidateListContextForDisplayedImage(
    const DisplayedImageLocation &location)
{
    const QUrl &displayedUrl = location.imageUrl();
    if (displayedUrl.isEmpty()) {
        return std::nullopt;
    }

    if (displayedLocationIsInsideOpenedCollectionScope(location)) {
        const QUrl currentUrl = normalizedImageUrl(displayedUrl);
        if (!currentUrl.isValid()) {
            return std::nullopt;
        }

        return ImageCandidateListContext::forOpenedCollectionScope(
            currentUrl, location.openedCollectionScope());
    }

    const DirectoryNavigationLocation navigationLocation
        = directoryNavigationLocationForFileUrl(displayedUrl);
    if (!navigationLocation.isValid()) {
        return std::nullopt;
    }

    return ImageCandidateListContext::forDirectory(
        navigationLocation.fileUrl, navigationLocation.directoryUrl);
}
}
