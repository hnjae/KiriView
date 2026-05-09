// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentstate.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagezoomstate.h"
#include "kiriview/src/imagedocumentstate.cxx.h"

#include <algorithm>
#include <utility>

namespace {
template <typename Value> bool replaceIfChanged(Value &current, const Value &next)
{
    if (current == next) {
        return false;
    }

    current = next;
    return true;
}

KiriView::RustImageDocumentStatus rustImageDocumentStatus(KiriView::ImageDocumentStatus status)
{
    switch (status) {
    case KiriView::ImageDocumentStatus::Null:
        return KiriView::RustImageDocumentStatus::Null;
    case KiriView::ImageDocumentStatus::Loading:
        return KiriView::RustImageDocumentStatus::Loading;
    case KiriView::ImageDocumentStatus::Ready:
        return KiriView::RustImageDocumentStatus::Ready;
    case KiriView::ImageDocumentStatus::Error:
        return KiriView::RustImageDocumentStatus::Error;
    }

    return KiriView::RustImageDocumentStatus::Null;
}

KiriView::ImageDocumentStatus imageDocumentStatus(KiriView::RustImageDocumentStatus status)
{
    switch (status) {
    case KiriView::RustImageDocumentStatus::Null:
        return KiriView::ImageDocumentStatus::Null;
    case KiriView::RustImageDocumentStatus::Loading:
        return KiriView::ImageDocumentStatus::Loading;
    case KiriView::RustImageDocumentStatus::Ready:
        return KiriView::ImageDocumentStatus::Ready;
    case KiriView::RustImageDocumentStatus::Error:
        return KiriView::ImageDocumentStatus::Error;
    }

    return KiriView::ImageDocumentStatus::Null;
}

KiriView::ImageDocumentChange imageDocumentChange(
    KiriView::RustImageDocumentNotificationChange change)
{
    switch (change) {
    case KiriView::RustImageDocumentNotificationChange::Status:
        return KiriView::ImageDocumentChange::Status;
    case KiriView::RustImageDocumentNotificationChange::Loading:
        return KiriView::ImageDocumentChange::Loading;
    case KiriView::RustImageDocumentNotificationChange::ImageSize:
        return KiriView::ImageDocumentChange::ImageSize;
    case KiriView::RustImageDocumentNotificationChange::ViewportSize:
        return KiriView::ImageDocumentChange::ViewportSize;
    case KiriView::RustImageDocumentNotificationChange::DisplaySize:
        return KiriView::ImageDocumentChange::DisplaySize;
    case KiriView::RustImageDocumentNotificationChange::ZoomPercent:
        return KiriView::ImageDocumentChange::ZoomPercent;
    case KiriView::RustImageDocumentNotificationChange::ZoomMode:
        return KiriView::ImageDocumentChange::ZoomMode;
    case KiriView::RustImageDocumentNotificationChange::MaximumManualZoomPercent:
        return KiriView::ImageDocumentChange::MaximumManualZoomPercent;
    case KiriView::RustImageDocumentNotificationChange::TwoPageMode:
        return KiriView::ImageDocumentChange::TwoPageMode;
    case KiriView::RustImageDocumentNotificationChange::RightToLeftReading:
        return KiriView::ImageDocumentChange::RightToLeftReading;
    case KiriView::RustImageDocumentNotificationChange::Repaint:
        return KiriView::ImageDocumentChange::Repaint;
    }

    return KiriView::ImageDocumentChange::Repaint;
}

std::vector<KiriView::ImageDocumentChange> imageDocumentChanges(
    rust::Vec<KiriView::RustImageDocumentNotificationChange> rustChanges)
{
    std::vector<KiriView::ImageDocumentChange> changes;
    changes.reserve(rustChanges.size());
    for (KiriView::RustImageDocumentNotificationChange change : rustChanges) {
        changes.push_back(imageDocumentChange(change));
    }
    return changes;
}

KiriView::RustImageDocumentZoomChangeSet rustImageDocumentZoomChangeSet(
    const KiriView::ImageZoomChangeSet &changes)
{
    return KiriView::RustImageDocumentZoomChangeSet { changes.imageSizeChanged,
        changes.viewportSizeChanged, changes.zoomModeChanged, changes.zoomPercentChanged,
        changes.displaySizeChanged, changes.maximumManualZoomPercentChanged };
}
}

namespace KiriView {
ImageDocumentState::ChangeBatch::ChangeBatch(ImageDocumentState &state)
    : m_state(&state)
{
    m_state->beginBatch();
}

ImageDocumentState::ChangeBatch::~ChangeBatch()
{
    if (m_state != nullptr) {
        m_state->endBatch();
    }
}

ImageDocumentState::ChangeBatch::ChangeBatch(ChangeBatch &&other) noexcept
    : m_state(other.m_state)
{
    other.m_state = nullptr;
}

ImageDocumentState::ImageDocumentState(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
}

ImageDocumentState::ChangeBatch ImageDocumentState::beginChangeBatch()
{
    return ChangeBatch(*this);
}

const QUrl &ImageDocumentState::sourceUrl() const { return m_sourceUrl; }

const DisplayedImageLocation &ImageDocumentState::displayedImageLocation() const
{
    return m_displayedImageLocation;
}

const ArchiveDocumentLocation &ImageDocumentState::displayedArchiveDocument() const
{
    return m_displayedImageLocation.archiveDocument();
}

const QUrl &ImageDocumentState::displayedUrl() const { return m_displayedImageLocation.imageUrl(); }

ImageDocumentStatus ImageDocumentState::status() const { return m_status; }

bool ImageDocumentState::loading() const { return m_loading; }

const QString &ImageDocumentState::errorString() const { return m_errorString; }

QString ImageDocumentState::windowTitleFileName() const
{
    return windowTitleFileNameForDisplayedLocation(m_displayedImageLocation);
}

const QUrl &ImageDocumentState::containerNavigationUrl() const { return m_containerNavigationUrl; }

const QUrl &ImageDocumentState::loadingContainerNavigationUrl() const
{
    return m_loadingContainerNavigationUrl;
}

bool ImageDocumentState::containerNavigationAvailable() const
{
    return !m_containerNavigationUrl.isEmpty();
}

void ImageDocumentState::setSourceUrl(const QUrl &sourceUrl)
{
    if (replaceIfChanged(m_sourceUrl, sourceUrl)) {
        notify(ImageDocumentChange::SourceUrl);
    }
}

void ImageDocumentState::setDisplayedImageLocation(const DisplayedImageLocation &location)
{
    replaceDisplayedImageLocation(location);
}

void ImageDocumentState::clearDisplayedImageUrls()
{
    replaceDisplayedImageLocation(DisplayedImageLocation {});
}

void ImageDocumentState::replaceDisplayedImageLocation(DisplayedImageLocation location)
{
    const QString previousWindowTitle = windowTitleFileName();
    const QUrl previousDisplayedUrl = displayedUrl();
    m_displayedImageLocation = std::move(location);
    if (previousDisplayedUrl != displayedUrl()) {
        notify(ImageDocumentChange::DisplayedUrl);
    }
    if (previousWindowTitle != windowTitleFileName()) {
        notify(ImageDocumentChange::WindowTitleFileName);
    }
}

void ImageDocumentState::setStatus(ImageDocumentStatus status)
{
    const RustImageDocumentStateSnapshot current
        = rustImageDocumentStateSnapshot(rustImageDocumentStatus(m_status), m_loading);
    const RustImageDocumentStateChange change
        = rustImageDocumentSetStatus(current, rustImageDocumentStatus(status));
    if (change.changed) {
        m_status = imageDocumentStatus(change.snapshot.status);
        m_loading = change.snapshot.loading;
        notify(ImageDocumentChange::Status);
    }
}

void ImageDocumentState::setLoading(bool loading)
{
    const RustImageDocumentStateSnapshot current
        = rustImageDocumentStateSnapshot(rustImageDocumentStatus(m_status), m_loading);
    const RustImageDocumentStateChange change = rustImageDocumentSetLoading(current, loading);
    if (change.changed) {
        m_status = imageDocumentStatus(change.snapshot.status);
        m_loading = change.snapshot.loading;
        notify(ImageDocumentChange::Loading);
    }
}

void ImageDocumentState::setErrorString(const QString &errorString)
{
    if (replaceIfChanged(m_errorString, errorString)) {
        notify(ImageDocumentChange::ErrorString);
    }
}

void ImageDocumentState::setContainerNavigationUrl(const QUrl &containerUrl)
{
    if (replaceIfChanged(m_containerNavigationUrl, containerUrl)) {
        notify(ImageDocumentChange::ContainerNavigation);
    }
}

void ImageDocumentState::setLoadingContainerNavigationUrl(const QUrl &containerUrl)
{
    m_loadingContainerNavigationUrl = containerUrl;
}

void ImageDocumentState::clearLoadingContainerNavigationUrl()
{
    m_loadingContainerNavigationUrl = QUrl();
}

void ImageDocumentState::beginBatch() { ++m_batchDepth; }

void ImageDocumentState::endBatch()
{
    if (m_batchDepth <= 0) {
        return;
    }

    --m_batchDepth;
    if (m_batchDepth > 0) {
        return;
    }

    std::vector<ImageDocumentChange> changes = std::move(m_pendingChanges);
    m_pendingChanges.clear();
    for (ImageDocumentChange change : changes) {
        emitChange(change);
    }
}

void ImageDocumentState::notify(ImageDocumentChange change)
{
    if (m_batchDepth > 0) {
        const bool alreadyPending
            = std::find(m_pendingChanges.cbegin(), m_pendingChanges.cend(), change)
            != m_pendingChanges.cend();
        if (!alreadyPending) {
            m_pendingChanges.push_back(change);
        }
        return;
    }

    emitChange(change);
}

void ImageDocumentState::emitChange(ImageDocumentChange change)
{
    invokeIfSet(m_changeCallback, change);
}

std::vector<ImageDocumentChange> imageDocumentSpreadTransitionNotifications()
{
    return imageDocumentChanges(rustImageDocumentSpreadTransitionNotifications());
}

std::vector<ImageDocumentChange> imageDocumentTwoPageModeNotifications()
{
    return imageDocumentChanges(rustImageDocumentTwoPageModeNotifications());
}

std::vector<ImageDocumentChange> imageDocumentSpreadZoomNotifications()
{
    return imageDocumentChanges(rustImageDocumentSpreadZoomNotifications());
}

std::vector<ImageDocumentChange> imageDocumentRightToLeftReadingNotifications(
    bool secondaryPageVisible)
{
    return imageDocumentChanges(
        rustImageDocumentRightToLeftReadingNotifications(secondaryPageVisible));
}

std::vector<ImageDocumentChange> imageDocumentPresentationZoomNotifications(
    const ImageZoomChangeSet &changes)
{
    return imageDocumentChanges(
        rustImageDocumentPresentationZoomNotifications(rustImageDocumentZoomChangeSet(changes)));
}
}
