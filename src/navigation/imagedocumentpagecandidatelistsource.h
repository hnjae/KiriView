// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATELISTSOURCE_H
#define KIRIVIEW_IMAGEDOCUMENTPAGECANDIDATELISTSOURCE_H

#include "imagedocumentpagenavigationtypes.h"
#include "location/imagelocation.h"

#include <QUrl>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace kiriview {
class ImageDocumentPageCandidateListSource
{
public:
    struct Directory
    {
        QUrl directoryUrl;
    };

    struct OpenedCollectionScope
    {
        OpenedCollectionScopeLocation openedCollectionScope;
    };

    static ImageDocumentPageCandidateListSource forDirectory(QUrl directoryUrl);
    static ImageDocumentPageCandidateListSource forOpenedCollectionScope(
        OpenedCollectionScopeLocation openedCollectionScope);

    OpenedCollectionScopeLocation openedCollectionScope() const;

    template <typename Visitor> decltype(auto) visit(Visitor&& visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), m_source);
    }

private:
    using Payload = std::variant<Directory, OpenedCollectionScope>;

    explicit ImageDocumentPageCandidateListSource(Payload source);

    Payload m_source;
};

bool sameImageDocumentPageCandidateListSource(const ImageDocumentPageCandidateListSource& left,
    const ImageDocumentPageCandidateListSource& right);

class ImageDocumentPageCandidateListContext
{
public:
    using DirectoryContext = ImageDocumentPageCandidateListSource::Directory;
    using OpenedCollectionScopeContext
        = ImageDocumentPageCandidateListSource::OpenedCollectionScope;

    static ImageDocumentPageCandidateListContext forDirectory(QUrl currentUrl, QUrl directoryUrl);
    static ImageDocumentPageCandidateListContext forOpenedCollectionScope(
        QUrl currentUrl, OpenedCollectionScopeLocation openedCollectionScope);
    static ImageDocumentPageCandidateListContext forSource(
        QUrl currentUrl, ImageDocumentPageCandidateListSource source);

    const QUrl& currentUrl() const;
    const ImageDocumentPageCandidateListSource& source() const;
    OpenedCollectionScopeLocation openedCollectionScope() const;

    template <typename Visitor> decltype(auto) visit(Visitor&& visitor) const
    {
        return m_source.visit(std::forward<Visitor>(visitor));
    }

private:
    explicit ImageDocumentPageCandidateListContext(
        QUrl currentUrl, ImageDocumentPageCandidateListSource source);

    QUrl m_currentUrl;
    ImageDocumentPageCandidateListSource m_source;
};

bool sameImageDocumentPageCandidateListContext(const ImageDocumentPageCandidateListContext& left,
    const ImageDocumentPageCandidateListContext& right);

struct ImageDocumentPageCandidateSnapshot
{
    ImageDocumentPageCandidateListSource source;
    std::vector<ImageDocumentPageCandidate> candidates;
};

bool imageDocumentPageCandidateSnapshotMatchesSource(
    const ImageDocumentPageCandidateSnapshot& snapshot,
    const ImageDocumentPageCandidateListSource& source);

std::optional<ImageDocumentPageCandidateListContext>
imageDocumentPageCandidateListContextForDisplayedImage(const DisplayedImageLocation& location);
}

#endif
