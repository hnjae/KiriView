// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadtypes.h"

#include <utility>

namespace KiriView {
ImageLoadSession::ImageLoadSession(quint64 id, ImageLoadRequest request,
    DisplayedImageLocation location, ImageFirstDisplayDecodeContext firstDisplay)
    : m_id(id)
    , m_request(std::move(request))
    , m_location(std::move(location))
    , m_firstDisplay(firstDisplay)
{
}

quint64 ImageLoadSession::id() const { return m_id; }

const ImageLoadRequest &ImageLoadSession::request() const { return m_request; }

const DisplayedImageLocation &ImageLoadSession::location() const { return m_location; }

const ImageFirstDisplayDecodeContext &ImageLoadSession::firstDisplay() const
{
    return m_firstDisplay;
}

const QUrl &ImageLoadSession::imageUrl() const { return m_location.imageUrl(); }

const ArchiveDocumentLocation &ImageLoadSession::archiveDocument() const
{
    return m_location.archiveDocument();
}

const QUrl &ImageLoadSession::containerNavigationUrl() const
{
    return m_request.containerNavigationUrl();
}

bool ImageLoadSession::hasContainerNavigationTarget() const
{
    return !containerNavigationUrl().isEmpty();
}

ImageDecodeRequest ImageLoadSession::decodeRequest() const
{
    return ImageDecodeRequest::fromLocation(m_id, m_location, m_firstDisplay);
}

bool ImageLoadSession::sameSession(const ImageLoadSession &session) const
{
    return m_id != 0 && m_id == session.m_id;
}

void ImageLoadSession::setImageUrl(const QUrl &url) { m_location.setImageUrl(url); }

void ImageLoadSession::setLocation(DisplayedImageLocation location)
{
    m_location = std::move(location);
}
}
