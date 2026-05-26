// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGELOADTYPES_H
#define KIRIVIEW_IMAGELOADTYPES_H

#include "decoding/imagedecoderequest.h"
#include "location/imagelocation.h"
#include "rendering/staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <utility>

namespace KiriView {
struct ImageLoadRequest {
    ImageLocation source;
    ImagePageScopeLocation displayedImagePageScope;
    ContainerLocation containerNavigation;

    static ImageLoadRequest fromUrl(QUrl sourceUrl, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            ImagePageScopeLocation::none(),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    static ImageLoadRequest fromLocation(QUrl sourceUrl,
        ImagePageScopeLocation displayedImagePageScope, QUrl containerNavigationUrl = QUrl())
    {
        return ImageLoadRequest { ImageLocation::fromUrl(std::move(sourceUrl)),
            std::move(displayedImagePageScope),
            ContainerLocation::fromUrl(std::move(containerNavigationUrl)) };
    }

    const QUrl &sourceUrl() const { return source.url(); }
    const ImagePageScopeLocation &imagePageScope() const { return displayedImagePageScope; }
    const QUrl &containerNavigationUrl() const { return containerNavigation.url(); }
    bool isEmpty() const { return source.isEmpty(); }
    bool isContainerNavigation() const { return !containerNavigation.isEmpty(); }
};

class ImageLoadSession
{
public:
    ImageLoadSession() = default;
    ImageLoadSession(quint64 id, ImageLoadRequest request, DisplayedImageLocation location,
        ImageFirstDisplayDecodeContext firstDisplay = {});

    quint64 id() const;
    const ImageLoadRequest &request() const;
    const DisplayedImageLocation &location() const;
    const ImageFirstDisplayDecodeContext &firstDisplay() const;
    const QUrl &imageUrl() const;
    const ImagePageScopeLocation &imagePageScope() const;
    const QUrl &containerNavigationUrl() const;
    bool hasContainerNavigationTarget() const;
    ImageDecodeRequest decodeRequest() const;
    bool sameSession(const ImageLoadSession &session) const;

    void setImageUrl(const QUrl &url);
    void setLocation(DisplayedImageLocation location);

private:
    quint64 m_id = 0;
    ImageLoadRequest m_request;
    DisplayedImageLocation m_location;
    ImageFirstDisplayDecodeContext m_firstDisplay;
};

enum class ImageLoadError {
    Generic,
    EmptyArchive,
};
}

#endif
