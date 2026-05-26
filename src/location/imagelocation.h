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

enum class ImagePageScopeKind {
    ComicBookArchive,
    GeneralArchive,
    Directory,
};

class ImagePageScopeLocation
{
public:
    ImagePageScopeLocation() = default;
    ImagePageScopeLocation(QUrl fileUrl, QUrl rootUrl, ImagePageScopeKind kind)
        : m_fileUrl(normalizedUrlForIdentity(fileUrl))
        , m_rootUrl(normalizedUrlForIdentity(rootUrl))
        , m_kind(kind)
    {
    }

    static ImagePageScopeLocation none() { return ImagePageScopeLocation(); }
    static ImagePageScopeLocation fromUrls(QUrl fileUrl, QUrl rootUrl, ImagePageScopeKind kind)
    {
        return ImagePageScopeLocation(std::move(fileUrl), std::move(rootUrl), kind);
    }

    const QUrl &fileUrl() const { return m_fileUrl; }
    const QUrl &rootUrl() const { return m_rootUrl; }
    ImagePageScopeKind kind() const { return m_kind; }
    bool isEmpty() const { return m_fileUrl.isEmpty() || m_rootUrl.isEmpty(); }
    bool isComicBook() const
    {
        return !isEmpty() && m_kind == ImagePageScopeKind::ComicBookArchive;
    }
    bool isDirectory() const { return !isEmpty() && m_kind == ImagePageScopeKind::Directory; }

    friend bool operator==(const ImagePageScopeLocation &left, const ImagePageScopeLocation &right)
    {
        return left.m_fileUrl == right.m_fileUrl && left.m_rootUrl == right.m_rootUrl
            && left.m_kind == right.m_kind;
    }
    friend bool operator!=(const ImagePageScopeLocation &left, const ImagePageScopeLocation &right)
    {
        return !(left == right);
    }

private:
    QUrl m_fileUrl;
    QUrl m_rootUrl;
    ImagePageScopeKind m_kind = ImagePageScopeKind::GeneralArchive;
};

bool sameImagePageScopeLocation(
    const ImagePageScopeLocation &left, const ImagePageScopeLocation &right);

class DisplayedImageLocation
{
public:
    DisplayedImageLocation() = default;
    DisplayedImageLocation(ImageLocation image, ImagePageScopeLocation imagePageScope)
        : m_image(std::move(image))
        , m_imagePageScope(std::move(imagePageScope))
    {
    }

    static DisplayedImageLocation fromUrl(QUrl imageUrl)
    {
        return DisplayedImageLocation { ImageLocation::fromUrl(std::move(imageUrl)),
            ImagePageScopeLocation::none() };
    }

    static DisplayedImageLocation fromUrl(QUrl imageUrl, ImagePageScopeLocation imagePageScope)
    {
        return DisplayedImageLocation { ImageLocation::fromUrl(std::move(imageUrl)),
            std::move(imagePageScope) };
    }

    static DisplayedImageLocation fromImagePageScope(
        QUrl imageUrl, ImagePageScopeLocation imagePageScope)
    {
        return fromUrl(std::move(imageUrl), std::move(imagePageScope));
    }

    const QUrl &imageUrl() const { return m_image.url(); }
    const ImagePageScopeLocation &imagePageScope() const { return m_imagePageScope; }
    const QUrl &imagePageScopeSourceUrl() const { return m_imagePageScope.fileUrl(); }
    const QUrl &imagePageScopeRootUrl() const { return m_imagePageScope.rootUrl(); }
    bool isEmpty() const { return m_image.isEmpty(); }
    void setImageUrl(QUrl url) { m_image = ImageLocation::fromUrl(std::move(url)); }
    void setImagePageScope(ImagePageScopeLocation imagePageScope)
    {
        m_imagePageScope = std::move(imagePageScope);
    }
    void clearImagePageScope() { m_imagePageScope = ImagePageScopeLocation::none(); }

    friend bool operator==(const DisplayedImageLocation &left, const DisplayedImageLocation &right)
    {
        return left.m_image == right.m_image && left.m_imagePageScope == right.m_imagePageScope;
    }
    friend bool operator!=(const DisplayedImageLocation &left, const DisplayedImageLocation &right)
    {
        return !(left == right);
    }

private:
    ImageLocation m_image;
    ImagePageScopeLocation m_imagePageScope;
};

}

#endif
