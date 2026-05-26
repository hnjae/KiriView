// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECANDIDATELISTSOURCE_H
#define KIRIVIEW_IMAGECANDIDATELISTSOURCE_H

#include "location/imagelocation.h"

#include <QUrl>
#include <optional>
#include <utility>
#include <variant>

namespace KiriView {
class ImageCandidateListSource
{
public:
    struct Directory {
        QUrl directoryUrl;
    };

    struct ImagePageScope {
        ImagePageScopeLocation imagePageScope;
    };

    static ImageCandidateListSource forDirectory(QUrl directoryUrl);
    static ImageCandidateListSource forImagePageScope(ImagePageScopeLocation imagePageScope);

    ImagePageScopeLocation imagePageScope() const;

    template <typename Visitor> decltype(auto) visit(Visitor &&visitor) const
    {
        return std::visit(std::forward<Visitor>(visitor), m_source);
    }

private:
    using Payload = std::variant<Directory, ImagePageScope>;

    explicit ImageCandidateListSource(Payload source);

    Payload m_source;
};

bool sameImageCandidateListSource(
    const ImageCandidateListSource &left, const ImageCandidateListSource &right);

class ImageCandidateListContext
{
public:
    using DirectoryContext = ImageCandidateListSource::Directory;
    using ImagePageScopeContext = ImageCandidateListSource::ImagePageScope;

    static ImageCandidateListContext forDirectory(QUrl currentUrl, QUrl directoryUrl);
    static ImageCandidateListContext forImagePageScope(
        QUrl currentUrl, ImagePageScopeLocation imagePageScope);
    static ImageCandidateListContext forSource(QUrl currentUrl, ImageCandidateListSource source);

    const QUrl &currentUrl() const;
    const ImageCandidateListSource &source() const;
    ImagePageScopeLocation imagePageScope() const;

    template <typename Visitor> decltype(auto) visit(Visitor &&visitor) const
    {
        return m_source.visit(std::forward<Visitor>(visitor));
    }

private:
    explicit ImageCandidateListContext(QUrl currentUrl, ImageCandidateListSource source);

    QUrl m_currentUrl;
    ImageCandidateListSource m_source;
};

bool sameImageCandidateListContext(
    const ImageCandidateListContext &left, const ImageCandidateListContext &right);

std::optional<ImageCandidateListContext> imageCandidateListContextForDisplayedImage(
    const DisplayedImageLocation &location);
}

#endif
