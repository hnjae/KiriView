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

std::optional<StaticDisplayImagePayload> DisplayedImageSurfaceState::displayImage() const
{
    if (!m_displayImage.has_value() || !m_displayImage->isValid()) {
        return std::nullopt;
    }

    return m_displayImage;
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::setImage(
    const QImage &image, bool predecodeCacheable)
{
    QImage displayImage = displayReadyImage(image);
    return replaceDisplayedImage(
        std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { std::move(displayImage) }),
        std::nullopt, std::nullopt, predecodeCacheable);
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::setStaticDisplayImage(
    StaticDisplayImagePayload displayImage, bool useFullImageSurface, bool predecodeCacheable,
    qsizetype tileCacheByteBudget)
{
    displayImage.image = displayReadyImage(displayImage.image);
    StaticImagePayload staticImage = displayImage.compatibilityStaticImage();

    std::optional<StaticImagePayload> storedStaticImage(std::move(staticImage));
    std::optional<StaticDisplayImagePayload> storedDisplayImage(std::move(displayImage));
    std::shared_ptr<DisplayedImageSurface> surface;
    if (useFullImageSurface) {
        surface = std::make_shared<DisplayedImageSurface>(
            LegacyFrameSurface { storedDisplayImage->image });
    } else {
        surface = std::make_shared<DisplayedImageSurface>(
            StaticTileSurface { *storedStaticImage, tileCacheByteBudget });
    }

    return replaceDisplayedImage(std::move(surface), std::move(storedStaticImage),
        std::move(storedDisplayImage), predecodeCacheable);
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
        std::move(surface), std::move(storedStaticImage), std::nullopt, predecodeCacheable);
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

    return replaceDisplayedImage(nullptr, std::nullopt, std::nullopt, false);
}

DisplayedImageSurfaceStateChange DisplayedImageSurfaceState::replaceDisplayedImage(
    std::shared_ptr<DisplayedImageSurface> surface, std::optional<StaticImagePayload> staticImage,
    std::optional<StaticDisplayImagePayload> displayImage, bool predecodeCacheable)
{
    m_surface = std::move(surface);
    m_staticImage = std::move(staticImage);
    m_displayImage = std::move(displayImage);
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
