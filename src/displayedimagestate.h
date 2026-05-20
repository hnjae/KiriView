// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_DISPLAYEDIMAGESTATE_H
#define KIRIVIEW_DISPLAYEDIMAGESTATE_H

#include "rendering/imagesurface.h"

#include <QImage>
#include <QSize>
#include <QSizeF>
#include <memory>
#include <optional>

namespace KiriView {
class DisplayedImageState
{
public:
    bool hasImage() const;
    std::shared_ptr<DisplayedImageSurface> imageSurface() const;
    QSize imageSize() const;
    quint64 revision() const;
    bool isPredecodeCacheable() const;
    std::optional<StaticImagePayload> staticImage() const;

    void setImage(const QImage &image, bool predecodeCacheable);
    void setStaticImage(StaticImagePayload staticImage, bool useFullImageSurface,
        bool predecodeCacheable, qsizetype tileCacheByteBudget);
    bool insertTile(DecodedTile tile);
    bool clear();

private:
    void replaceDisplayedImage(std::shared_ptr<DisplayedImageSurface> surface,
        std::optional<StaticImagePayload> staticImage, bool predecodeCacheable);
    void finishImageChange();

    std::shared_ptr<DisplayedImageSurface> m_surface;
    std::optional<StaticImagePayload> m_staticImage;
    bool m_imageIsPredecodeCacheable = false;
    quint64 m_imageRevision = 0;
};
}

#endif
