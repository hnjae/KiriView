// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageloadtypes.h"

#include <utility>

namespace KiriView {
ImageLoadSession::ImageLoadSession(quint64 id, ImageLoadRequest request,
    DisplayedImageLocation location, ImageFirstDisplayDecodeContext firstDisplay)
    : m_id(id)
    , m_request(std::move(request))
    , m_kind(m_request.kind())
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

ImageDocumentPageKind ImageLoadSession::kind() const { return m_kind; }

const OpenedCollectionScopeLocation &ImageLoadSession::openedCollectionScope() const
{
    return m_location.openedCollectionScope();
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

void ImageLoadSession::setImageDocumentPageCandidate(const ImageDocumentPageCandidate &candidate)
{
    setImageTarget(ImageDocumentPageTarget { candidate.url, candidate.kind });
}

void ImageLoadSession::setImageTarget(const ImageDocumentPageTarget &target)
{
    m_location.setImageUrl(target.url);
    m_kind = target.kind;
}

void ImageLoadSession::setImageUrl(const QUrl &url)
{
    setImageTarget(ImageDocumentPageTarget { url, ImageDocumentPageKind::Image });
}

void ImageLoadSession::setLocation(DisplayedImageLocation location)
{
    m_location = std::move(location);
}
}
