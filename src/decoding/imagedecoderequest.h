// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDECODEREQUEST_H
#define KIRIVIEW_IMAGEDECODEREQUEST_H

#include "location/imagelocation.h"
#include "location/imageurl.h"
#include "rendering/staticimage.h"

#include <QUrl>
#include <QtGlobal>
#include <utility>

namespace kiriview {
class ImageDecodeRequest
{
public:
    ImageDecodeRequest() = default;

    static ImageDecodeRequest fromUrl(
        quint64 id, QUrl imageUrl, ImageFirstDisplayDecodeContext firstDisplay = {})
    {
        return fromLocation(id, DisplayedImageLocation::fromUrl(std::move(imageUrl)), firstDisplay);
    }

    static ImageDecodeRequest fromLocation(quint64 id, DisplayedImageLocation location,
        ImageFirstDisplayDecodeContext firstDisplay = {})
    {
        return ImageDecodeRequest(id, std::move(location), firstDisplay);
    }

    quint64 id() const { return m_id; }
    const DisplayedImageLocation& location() const { return m_location; }
    const QUrl& imageUrl() const { return m_location.imageUrl(); }
    const OpenedCollectionScopeLocation& openedCollectionScope() const
    {
        return m_location.openedCollectionScope();
    }
    const ImageFirstDisplayDecodeContext& firstDisplay() const { return m_firstDisplay; }
    bool isEmpty() const { return m_location.isEmpty(); }
    bool matches(quint64 id, const QUrl& imageUrl) const
    {
        return m_id == id && sameNormalizedUrl(m_location.imageUrl(), imageUrl);
    }
    bool matches(const ImageDecodeRequest& request) const
    {
        return matches(request.id(), request.imageUrl());
    }

private:
    ImageDecodeRequest(
        quint64 id, DisplayedImageLocation location, ImageFirstDisplayDecodeContext firstDisplay)
        : m_id(id)
        , m_location(std::move(location))
        , m_firstDisplay(firstDisplay)
    {
    }

    quint64 m_id = 0;
    DisplayedImageLocation m_location;
    ImageFirstDisplayDecodeContext m_firstDisplay;
};
}

#endif
