// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DISPLAYEDIMAGESURFACESTATE_H
#define KIRIVIEW_DISPLAYEDIMAGESURFACESTATE_H

#include "imagesurface.h"

#include <QImage>
#include <QSize>
#include <QSizeF>
#include <memory>
#include <optional>

namespace KiriView {
struct DisplayedImageSurfaceStateChange {
    QSize imageSize;
    quint64 revision = 0;
};

class DisplayedImageSurfaceState
{
public:
    bool hasImage() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    QSize imageSize() const;
    quint64 revision() const;
    bool isPredecodeCacheable() const;
    std::optional<StaticImagePayload> staticImage() const;

    DisplayedImageSurfaceStateChange setImage(const QImage &image, bool predecodeCacheable);
    DisplayedImageSurfaceStateChange setStaticImage(StaticImagePayload staticImage,
        bool useFullImageSurface, bool predecodeCacheable, qsizetype tileCacheByteBudget);
    std::optional<DisplayedImageSurfaceStateChange> insertTile(DecodedTile tile);
    std::optional<DisplayedImageSurfaceStateChange> clearTiles();
    std::optional<DisplayedImageSurfaceStateChange> clear();

private:
    DisplayedImageSurfaceStateChange replaceDisplayedImage(
        std::shared_ptr<DisplayedImageSurface> surface,
        std::optional<StaticImagePayload> staticImage, bool predecodeCacheable);
    DisplayedImageSurfaceStateChange finishImageChange();

    std::shared_ptr<DisplayedImageSurface> m_surface;
    std::optional<StaticImagePayload> m_staticImage;
    bool m_imageIsPredecodeCacheable = false;
    quint64 m_imageRevision = 0;
};
}

#endif
