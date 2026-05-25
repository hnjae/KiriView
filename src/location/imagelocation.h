// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOCATION_H
#define KIRIVIEW_IMAGELOCATION_H

#include "location/imageurl.h"

#include <QUrl>
#include <utility>

namespace KiriView {
class ImageLocation
{
public:
    ImageLocation() = default;
    explicit ImageLocation(QUrl url)
        : m_url(normalizedUrlForIdentity(url))
    {
    }

    static ImageLocation fromUrl(QUrl url) { return ImageLocation(std::move(url)); }

    const QUrl &url() const { return m_url; }
    bool isEmpty() const { return m_url.isEmpty(); }

    friend bool operator==(const ImageLocation &left, const ImageLocation &right)
    {
        return left.m_url == right.m_url;
    }
    friend bool operator!=(const ImageLocation &left, const ImageLocation &right)
    {
        return !(left == right);
    }

private:
    QUrl m_url;
};

class ContainerLocation
{
public:
    ContainerLocation() = default;
    explicit ContainerLocation(QUrl url)
        : m_url(normalizedUrlForIdentity(url))
    {
    }

    static ContainerLocation none() { return ContainerLocation(); }
    static ContainerLocation fromUrl(QUrl url) { return ContainerLocation(std::move(url)); }

    const QUrl &url() const { return m_url; }
    bool isEmpty() const { return m_url.isEmpty(); }

    friend bool operator==(const ContainerLocation &left, const ContainerLocation &right)
    {
        return left.m_url == right.m_url;
    }
    friend bool operator!=(const ContainerLocation &left, const ContainerLocation &right)
    {
        return !(left == right);
    }

private:
    QUrl m_url;
};

enum class ArchiveDocumentKind {
    ComicBook,
    General,
    Directory,
};

class ArchiveDocumentLocation
{
public:
    ArchiveDocumentLocation() = default;
    ArchiveDocumentLocation(QUrl fileUrl, QUrl rootUrl, ArchiveDocumentKind kind)
        : m_fileUrl(normalizedUrlForIdentity(fileUrl))
        , m_rootUrl(normalizedUrlForIdentity(rootUrl))
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
    bool isDirectory() const { return !isEmpty() && m_kind == ArchiveDocumentKind::Directory; }

    friend bool operator==(
        const ArchiveDocumentLocation &left, const ArchiveDocumentLocation &right)
    {
        return left.m_fileUrl == right.m_fileUrl && left.m_rootUrl == right.m_rootUrl
            && left.m_kind == right.m_kind;
    }
    friend bool operator!=(
        const ArchiveDocumentLocation &left, const ArchiveDocumentLocation &right)
    {
        return !(left == right);
    }

private:
    QUrl m_fileUrl;
    QUrl m_rootUrl;
    ArchiveDocumentKind m_kind = ArchiveDocumentKind::General;
};

bool sameArchiveDocumentLocation(
    const ArchiveDocumentLocation &left, const ArchiveDocumentLocation &right);

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

    static DisplayedImageLocation fromUrl(QUrl imageUrl, ArchiveDocumentLocation archiveDocument)
    {
        return DisplayedImageLocation { ImageLocation::fromUrl(std::move(imageUrl)),
            std::move(archiveDocument) };
    }

    static DisplayedImageLocation fromArchiveDocument(
        QUrl imageUrl, ArchiveDocumentLocation archiveDocument)
    {
        return fromUrl(std::move(imageUrl), std::move(archiveDocument));
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

    friend bool operator==(const DisplayedImageLocation &left, const DisplayedImageLocation &right)
    {
        return left.m_image == right.m_image && left.m_archiveDocument == right.m_archiveDocument;
    }
    friend bool operator!=(const DisplayedImageLocation &left, const DisplayedImageLocation &right)
    {
        return !(left == right);
    }

private:
    ImageLocation m_image;
    ArchiveDocumentLocation m_archiveDocument;
};

}

#endif
