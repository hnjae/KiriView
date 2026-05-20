// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecandidatelistsource.h"

#include "imagecontainer.h"
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
    const KiriView::ImageCandidateListSource::ArchiveDocument &left,
    const KiriView::ImageCandidateListSource::ArchiveDocument &right)
{
    return KiriView::sameArchiveDocumentLocation(left.archiveDocument, right.archiveDocument);
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

ImageCandidateListSource ImageCandidateListSource::forArchiveDocument(
    ArchiveDocumentLocation archiveDocument)
{
    return ImageCandidateListSource { ArchiveDocument { std::move(archiveDocument) } };
}

ArchiveDocumentLocation ImageCandidateListSource::archiveDocument() const
{
    const auto *archiveSource = std::get_if<ArchiveDocument>(&m_source);
    return archiveSource == nullptr ? ArchiveDocumentLocation::none()
                                    : archiveSource->archiveDocument;
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

ImageCandidateListContext ImageCandidateListContext::forArchiveDocument(
    QUrl currentUrl, ArchiveDocumentLocation archiveDocument)
{
    return ImageCandidateListContext {
        std::move(currentUrl),
        ImageCandidateListSource::forArchiveDocument(std::move(archiveDocument)),
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

ArchiveDocumentLocation ImageCandidateListContext::archiveDocument() const
{
    return m_source.archiveDocument();
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

    if (displayedLocationIsInsideArchiveDocument(location)) {
        const QUrl currentUrl = normalizedImageUrl(displayedUrl);
        if (!currentUrl.isValid()) {
            return std::nullopt;
        }

        return ImageCandidateListContext::forArchiveDocument(
            currentUrl, location.archiveDocument());
    }

    const QUrl currentUrl = navigationSourceUrl(displayedUrl);
    const QUrl parentUrl = currentUrl.adjusted(QUrl::RemoveFilename | QUrl::NormalizePathSegments);
    if (!currentUrl.isValid() || currentUrl.isEmpty() || !parentUrl.isValid()
        || parentUrl.isEmpty()) {
        return std::nullopt;
    }

    return ImageCandidateListContext::forDirectory(currentUrl, parentUrl);
}
}
