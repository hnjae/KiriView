// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "rendering/displayedimagesurfacestate.h"

#include "rendering/imagerendering.h"

#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
bool DisplayedImageSurfaceState::hasImage() const
{
    return m_surface != nullptr && !m_surface->isNull();
}

std::shared_ptr<DisplayedImageSurface> DisplayedImageSurfaceState::imageSurface() const
{
    return m_surface;
}

QSize DisplayedImageSurfaceState::imageSize() const
{
    if (m_surface != nullptr) {
        return m_surface->imageSize();
    }

    return {};
}

quint64 DisplayedImageSurfaceState::revision() const { return m_imageRevision; }

bool DisplayedImageSurfaceState::isPredecodeCacheable() const
{
    return m_imageIsPredecodeCacheable;
}

std::optional<StaticImagePayload> DisplayedImageSurfaceState::staticImage() const
{
    if (!m_staticImage.has_value() || !m_staticImage->isValid()) {
        return std::nullopt;
    }

    return m_staticImage;
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::setImage(
    const QImage &image, bool predecodeCacheable)
{
    QImage displayImage = displayReadyImage(image);
    return replaceDisplayedImage(
        std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { std::move(displayImage) }),
        std::nullopt, predecodeCacheable);
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::setStaticImage(
    StaticImagePayload staticImage, bool useFullImageSurface, bool predecodeCacheable,
    qsizetype tileCacheByteBudget)
{
    QImage displayImage = displayReadyImage(staticImage.preview);
    staticImage.preview = displayImage;

    std::optional<StaticImagePayload> storedStaticImage(std::move(staticImage));
    std::shared_ptr<DisplayedImageSurface> surface;
    if (useFullImageSurface) {
        surface = std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { displayImage });
    } else {
        surface = std::make_shared<DisplayedImageSurface>(
            StaticTileSurface { *storedStaticImage, tileCacheByteBudget });
    }

    return replaceDisplayedImage(
        std::move(surface), std::move(storedStaticImage), predecodeCacheable);
}

std::optional<DisplayedImageSurfaceStateChange> DisplayedImageSurfaceState::insertTile(
    DecodedTile tile)
{
    if (m_surface == nullptr) {
        return std::nullopt;
    }
    auto *surface = m_surface->staticTileSurface();
    if (surface == nullptr) {
        return std::nullopt;
    }

    if (!surface->insertTile(std::move(tile))) {
        return std::nullopt;
    }

    return finishImageChange();
}

std::optional<DisplayedImageSurfaceStateChange> DisplayedImageSurfaceState::clearTiles()
{
    if (m_surface == nullptr) {
        return std::nullopt;
    }

    auto *surface = m_surface->staticTileSurface();
    if (surface == nullptr || surface->tiles().empty()) {
        return std::nullopt;
    }

    surface->clearTiles();
    return finishImageChange();
}

std::optional<DisplayedImageSurfaceStateChange> DisplayedImageSurfaceState::clear()
{
    if (m_surface == nullptr) {
        return std::nullopt;
    }

    return replaceDisplayedImage(nullptr, std::nullopt, false);
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::replaceDisplayedImage(
    std::shared_ptr<DisplayedImageSurface> surface, std::optional<StaticImagePayload> staticImage,
    bool predecodeCacheable)
{
    m_surface = std::move(surface);
    m_staticImage = std::move(staticImage);
    m_imageIsPredecodeCacheable = predecodeCacheable;
    return finishImageChange();
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::finishImageChange()
{
    ++m_imageRevision;
    return DisplayedImageSurfaceStateChange {
        imageSize(),
        m_imageRevision,
    };
}
}
