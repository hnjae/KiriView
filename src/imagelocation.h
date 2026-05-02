// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOCATION_H
#define KIRIVIEW_IMAGELOCATION_H

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

class ComicBookRootLocation
{
public:
    ComicBookRootLocation() = default;
    explicit ComicBookRootLocation(QUrl url)
        : m_url(std::move(url))
    {
    }

    static ComicBookRootLocation none() { return ComicBookRootLocation(); }
    static ComicBookRootLocation fromUrl(QUrl url) { return ComicBookRootLocation(std::move(url)); }

    const QUrl &url() const { return m_url; }
    bool isEmpty() const { return m_url.isEmpty(); }

private:
    QUrl m_url;
};

class DisplayedImageLocation
{
public:
    DisplayedImageLocation() = default;
    DisplayedImageLocation(ImageLocation image, ComicBookRootLocation comicBookRoot)
        : m_image(std::move(image))
        , m_comicBookRoot(std::move(comicBookRoot))
    {
    }

    static DisplayedImageLocation fromUrls(QUrl imageUrl, QUrl comicBookRootUrl = QUrl())
    {
        return DisplayedImageLocation { ImageLocation::fromUrl(std::move(imageUrl)),
            ComicBookRootLocation::fromUrl(std::move(comicBookRootUrl)) };
    }

    const QUrl &imageUrl() const { return m_image.url(); }
    const QUrl &comicBookRootUrl() const { return m_comicBookRoot.url(); }
    bool isEmpty() const { return m_image.isEmpty(); }
    void setImageUrl(QUrl url) { m_image = ImageLocation::fromUrl(std::move(url)); }
    void setComicBookRootUrl(QUrl url)
    {
        m_comicBookRoot = ComicBookRootLocation::fromUrl(std::move(url));
    }

private:
    ImageLocation m_image;
    ComicBookRootLocation m_comicBookRoot;
};

struct ImageLoadRequest {
    ImageLocation source;
    ComicBookRootLocation displayedComicBookRoot;
    ContainerLocation containerNavigation;

    static ImageLoadRequest fromUrls(
        QUrl sourceUrl, QUrl displayedComicBookRootUrl, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            ComicBookRootLocation::fromUrl(std::move(displayedComicBookRootUrl)),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    const QUrl &sourceUrl() const { return source.url(); }
    const QUrl &displayedComicBookRootUrl() const { return displayedComicBookRoot.url(); }
    const QUrl &containerNavigationUrl() const { return containerNavigation.url(); }
    bool isEmpty() const { return source.isEmpty(); }
    bool isContainerNavigation() const { return !containerNavigation.isEmpty(); }
};

struct PredecodedImage {
    QImage image;
    DisplayedImageLocation location;
};
}

#endif
