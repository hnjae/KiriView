// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "displayedimagestate.h"

#include "imagerendering.h"

#include <memory>
#include <optional>
#include <utility>

namespace KiriView {
bool DisplayedImageState::hasImage() const { return m_surface != nullptr && !m_surface->isNull(); }

std::shared_ptr<DisplayedImageSurface> DisplayedImageState::imageSurface() const
{
    return m_surface;
}

QSize DisplayedImageState::imageSize() const
{
    if (m_surface != nullptr) {
        return m_surface->imageSize();
    }

    return {};
}

quint64 DisplayedImageState::revision() const { return m_imageRevision; }

bool DisplayedImageState::isPredecodeCacheable() const { return m_imageIsPredecodeCacheable; }

std::optional<StaticImagePayload> DisplayedImageState::staticImage() const
{
    if (!m_staticImage.has_value() || !m_staticImage->isValid()) {
        return std::nullopt;
    }

    return m_staticImage;
}

void DisplayedImageState::setImage(const QImage &image, bool predecodeCacheable)
{
    QImage displayImage = displayReadyImage(image);
    replaceDisplayedImage(
        std::make_shared<DisplayedImageSurface>(LegacyFrameSurface { std::move(displayImage) }),
        std::nullopt, predecodeCacheable);
}

void DisplayedImageState::setStaticImage(StaticImagePayload staticImage, bool useFullImageSurface,
    bool predecodeCacheable, qsizetype tileCacheByteBudget)
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

    replaceDisplayedImage(std::move(surface), std::move(storedStaticImage), predecodeCacheable);
}

bool DisplayedImageState::insertTile(DecodedTile tile)
{
    if (m_surface == nullptr) {
        return false;
    }
    auto *surface = m_surface->staticTileSurface();
    if (surface == nullptr) {
        return false;
    }

    if (!surface->insertTile(std::move(tile))) {
        return false;
    }

    finishImageChange();
    return true;
}

bool DisplayedImageState::clear()
{
    if (m_surface == nullptr) {
        return false;
    }

    replaceDisplayedImage(nullptr, std::nullopt, false);
    return true;
}

void DisplayedImageState::replaceDisplayedImage(std::shared_ptr<DisplayedImageSurface> surface,
    std::optional<StaticImagePayload> staticImage, bool predecodeCacheable)
{
    m_surface = std::move(surface);
    m_staticImage = std::move(staticImage);
    m_imageIsPredecodeCacheable = predecodeCacheable;
    finishImageChange();
}

void DisplayedImageState::finishImageChange() { ++m_imageRevision; }
}
