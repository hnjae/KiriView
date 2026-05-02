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

struct DisplayedImageLocation {
    QUrl imageUrl;
    QUrl comicBookRootUrl;

    bool isEmpty() const { return imageUrl.isEmpty(); }
};

struct ImageLoadRequest {
    ImageLocation source;
    QUrl displayedComicBookRootUrl;
    ContainerLocation containerNavigation;

    static ImageLoadRequest fromUrls(
        QUrl sourceUrl, QUrl displayedComicBookRootUrl, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            std::move(displayedComicBookRootUrl),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    const QUrl &sourceUrl() const { return source.url(); }
    const QUrl &containerNavigationUrl() const { return containerNavigation.url(); }
    bool isEmpty() const { return source.isEmpty(); }
    bool isContainerNavigation() const { return !containerNavigation.isEmpty(); }
};

struct ContainerImageOpenRequest {
    QUrl imageUrl;
    QUrl containerNavigationUrl;
};

struct PredecodedImage {
    QImage image;
    DisplayedImageLocation location;
};
}

#endif
