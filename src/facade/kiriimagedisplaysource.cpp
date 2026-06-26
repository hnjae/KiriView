// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "facade/kiriimagedisplaysource.h"

#include <utility>

namespace {
KiriImageDisplaySource::PageRole pageRole(kiriview::DisplayedPageRole role)
{
    switch (role) {
    case kiriview::DisplayedPageRole::Primary:
        return KiriImageDisplaySource::PageRole::Primary;
    case kiriview::DisplayedPageRole::Secondary:
        return KiriImageDisplaySource::PageRole::Secondary;
    }

    return KiriImageDisplaySource::PageRole::Primary;
}

KiriImageDisplaySource::ScopeKind scopeKind(kiriview::ImagePresentationScopeKey::Kind kind)
{
    switch (kind) {
    case kiriview::ImagePresentationScopeKey::Kind::Empty:
        return KiriImageDisplaySource::ScopeKind::Empty;
    case kiriview::ImagePresentationScopeKey::Kind::DirectImage:
        return KiriImageDisplaySource::ScopeKind::DirectImage;
    case kiriview::ImagePresentationScopeKey::Kind::OpenedCollection:
        return KiriImageDisplaySource::ScopeKind::OpenedCollection;
    }

    return KiriImageDisplaySource::ScopeKind::Empty;
}

KiriImageDisplaySource::Quality quality(kiriview::DisplayImageQuality quality)
{
    switch (quality) {
    case kiriview::DisplayImageQuality::Exact:
        return KiriImageDisplaySource::Quality::Exact;
    case kiriview::DisplayImageQuality::FirstDisplay:
        return KiriImageDisplaySource::Quality::FirstDisplay;
    case kiriview::DisplayImageQuality::ThumbnailPreview:
        return KiriImageDisplaySource::Quality::ThumbnailPreview;
    case kiriview::DisplayImageQuality::BoundedDetail:
        return KiriImageDisplaySource::Quality::BoundedDetail;
    case kiriview::DisplayImageQuality::Unsupported:
        return KiriImageDisplaySource::Quality::Unsupported;
    case kiriview::DisplayImageQuality::Failed:
        return KiriImageDisplaySource::Quality::Failed;
    }

    return KiriImageDisplaySource::Quality::Exact;
}

KiriImageDisplaySource::Status status(kiriview::ImageDisplaySourceStatus status)
{
    switch (status) {
    case kiriview::ImageDisplaySourceStatus::Missing:
        return KiriImageDisplaySource::Status::Missing;
    case kiriview::ImageDisplaySourceStatus::Ready:
        return KiriImageDisplaySource::Status::Ready;
    case kiriview::ImageDisplaySourceStatus::Error:
        return KiriImageDisplaySource::Status::Error;
    case kiriview::ImageDisplaySourceStatus::Unsupported:
        return KiriImageDisplaySource::Status::Unsupported;
    }

    return KiriImageDisplaySource::Status::Missing;
}

KiriImageDisplaySource::RetentionStatus retentionStatus(
    kiriview::ImageDisplaySourceRetentionStatus status)
{
    switch (status) {
    case kiriview::ImageDisplaySourceRetentionStatus::None:
        return KiriImageDisplaySource::RetentionStatus::None;
    case kiriview::ImageDisplaySourceRetentionStatus::StaleRetained:
        return KiriImageDisplaySource::RetentionStatus::StaleRetained;
    }

    return KiriImageDisplaySource::RetentionStatus::None;
}

bool projectionsEqual(const kiriview::ImageDisplaySourceProjection& left,
    const kiriview::ImageDisplaySourceProjection& right)
{
    return left.visible == right.visible && left.pageRole == right.pageRole
        && left.providerUrl == right.providerUrl && left.revision == right.revision
        && left.revisionToken == right.revisionToken && left.sourceIdentity == right.sourceIdentity
        && left.selectedSourceScope == right.selectedSourceScope
        && left.originalSize == right.originalSize && left.rasterSize == right.rasterSize
        && left.sourceSizeHint == right.sourceSizeHint && left.quality == right.quality
        && left.status == right.status && left.cacheEnabled == right.cacheEnabled
        && left.loadAcknowledgmentRequired == right.loadAcknowledgmentRequired
        && left.displaySize == right.displaySize && left.visibleItemRect == right.visibleItemRect
        && left.rotationDegrees == right.rotationDegrees
        && left.retentionStatus == right.retentionStatus
        && left.retainWhileLoadingEligible == right.retainWhileLoadingEligible;
}
}

KiriImageDisplaySource::KiriImageDisplaySource(kiriview::DisplayedPageRole role, QObject* parent)
    : QObject(parent)
{
    m_projection.pageRole = role;
    m_projection.revisionToken = kiriview::imageDisplaySourceRevisionToken(m_projection.revision);
}

bool KiriImageDisplaySource::visible() const { return m_projection.visible; }

KiriImageDisplaySource::PageRole KiriImageDisplaySource::pageRole() const
{
    return ::pageRole(m_projection.pageRole);
}

QUrl KiriImageDisplaySource::providerUrl() const { return m_projection.providerUrl; }

QString KiriImageDisplaySource::revisionToken() const { return m_projection.revisionToken; }

QString KiriImageDisplaySource::sourceIdentity() const { return m_projection.sourceIdentity; }

KiriImageDisplaySource::ScopeKind KiriImageDisplaySource::selectedSourceScopeKind() const
{
    return scopeKind(m_projection.selectedSourceScope.kind);
}

QUrl KiriImageDisplaySource::selectedSourceScopeUrl() const
{
    return m_projection.selectedSourceScope.url;
}

QSize KiriImageDisplaySource::originalSize() const { return m_projection.originalSize; }

QSize KiriImageDisplaySource::rasterSize() const { return m_projection.rasterSize; }

QSize KiriImageDisplaySource::sourceSizeHint() const { return m_projection.sourceSizeHint; }

KiriImageDisplaySource::Quality KiriImageDisplaySource::quality() const
{
    return ::quality(m_projection.quality);
}

KiriImageDisplaySource::Status KiriImageDisplaySource::status() const
{
    return ::status(m_projection.status);
}

bool KiriImageDisplaySource::cacheEnabled() const { return m_projection.cacheEnabled; }

bool KiriImageDisplaySource::loadAcknowledgmentRequired() const
{
    return m_projection.loadAcknowledgmentRequired;
}

int KiriImageDisplaySource::rotationDegrees() const { return m_projection.rotationDegrees; }

KiriImageDisplaySource::RetentionStatus KiriImageDisplaySource::retentionStatus() const
{
    return ::retentionStatus(m_projection.retentionStatus);
}

bool KiriImageDisplaySource::retainWhileLoadingEligible() const
{
    return m_projection.retainWhileLoadingEligible;
}

void KiriImageDisplaySource::setProjection(kiriview::ImageDisplaySourceProjection projection)
{
    if (projectionsEqual(m_projection, projection)) {
        return;
    }

    m_projection = std::move(projection);
    Q_EMIT changed();
}
