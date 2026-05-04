// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOCATION_H
#define KIRIVIEW_IMAGELOCATION_H

#include "imagesurface.h"

#include <QImage>
#include <QUrl>
#include <utility>

namespace KiriView {
class ImageLocation
{
public:
    ImageLocation() = default;
    explicit ImageLocation(QUrl url)
        : m_url(std::move(url))
    {
    }

    static ImageLocation fromUrl(QUrl url) { return ImageLocation(std::move(url)); }

    const QUrl &url() const { return m_url; }
    bool isEmpty() const { return m_url.isEmpty(); }

private:
    QUrl m_url;
};

class ContainerLocation
{
public:
    ContainerLocation() = default;
    explicit ContainerLocation(QUrl url)
        : m_url(std::move(url))
    {
    }

    static ContainerLocation none() { return ContainerLocation(); }
    static ContainerLocation fromUrl(QUrl url) { return ContainerLocation(std::move(url)); }

    const QUrl &url() const { return m_url; }
    bool isEmpty() const { return m_url.isEmpty(); }

private:
    QUrl m_url;
};

enum class ArchiveDocumentKind {
    ComicBook,
    General,
};

class ArchiveDocumentLocation
{
public:
    ArchiveDocumentLocation() = default;
    ArchiveDocumentLocation(QUrl fileUrl, QUrl rootUrl, ArchiveDocumentKind kind)
        : m_fileUrl(std::move(fileUrl))
        , m_rootUrl(std::move(rootUrl))
        , m_kind(kind)
    {
    }

    static ArchiveDocumentLocation none() { return ArchiveDocumentLocation(); }
    static ArchiveDocumentLocation fromUrls(QUrl fileUrl, QUrl rootUrl, ArchiveDocumentKind kind)
    {
        return ArchiveDocumentLocation(std::move(fileUrl), std::move(rootUrl), kind);
    }

    const QUrl &fileUrl() const { return m_fileUrl; }
    const QUrl &rootUrl() const { return m_rootUrl; }
    ArchiveDocumentKind kind() const { return m_kind; }
    bool isEmpty() const { return m_fileUrl.isEmpty() || m_rootUrl.isEmpty(); }
    bool isComicBook() const { return !isEmpty() && m_kind == ArchiveDocumentKind::ComicBook; }

private:
    QUrl m_fileUrl;
    QUrl m_rootUrl;
    ArchiveDocumentKind m_kind = ArchiveDocumentKind::General;
};

class DisplayedImageLocation
{
public:
    DisplayedImageLocation() = default;
    DisplayedImageLocation(ImageLocation image, ArchiveDocumentLocation archiveDocument)
        : m_image(std::move(image))
        , m_archiveDocument(std::move(archiveDocument))
    {
    }

    static DisplayedImageLocation fromUrl(QUrl imageUrl)
    {
        return DisplayedImageLocation { ImageLocation::fromUrl(std::move(imageUrl)),
            ArchiveDocumentLocation::none() };
    }

    static DisplayedImageLocation fromArchiveDocument(
        QUrl imageUrl, ArchiveDocumentLocation archiveDocument)
    {
        return DisplayedImageLocation { ImageLocation::fromUrl(std::move(imageUrl)),
            std::move(archiveDocument) };
    }

    const QUrl &imageUrl() const { return m_image.url(); }
    const ArchiveDocumentLocation &archiveDocument() const { return m_archiveDocument; }
    const QUrl &archiveDocumentFileUrl() const { return m_archiveDocument.fileUrl(); }
    const QUrl &archiveDocumentRootUrl() const { return m_archiveDocument.rootUrl(); }
    bool isEmpty() const { return m_image.isEmpty(); }
    void setImageUrl(QUrl url) { m_image = ImageLocation::fromUrl(std::move(url)); }
    void setArchiveDocument(ArchiveDocumentLocation archiveDocument)
    {
        m_archiveDocument = std::move(archiveDocument);
    }
    void clearArchiveDocument() { m_archiveDocument = ArchiveDocumentLocation::none(); }

private:
    ImageLocation m_image;
    ArchiveDocumentLocation m_archiveDocument;
};

struct ImageLoadRequest {
    ImageLocation source;
    ArchiveDocumentLocation displayedArchiveDocument;
    ContainerLocation containerNavigation;

    static ImageLoadRequest fromUrl(QUrl sourceUrl, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            ArchiveDocumentLocation::none(),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    static ImageLoadRequest fromLocation(QUrl sourceUrl,
        ArchiveDocumentLocation displayedArchiveDocument, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            std::move(displayedArchiveDocument),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    const QUrl &sourceUrl() const { return source.url(); }
    const ArchiveDocumentLocation &archiveDocument() const { return displayedArchiveDocument; }
    const QUrl &containerNavigationUrl() const { return containerNavigation.url(); }
    bool isEmpty() const { return source.isEmpty(); }
    bool isContainerNavigation() const { return !containerNavigation.isEmpty(); }
};

struct PredecodedImage {
    std::shared_ptr<ImageTileSource> source;
    QImage preview;
    DisplayedImageLocation location;
};
}

#endif
