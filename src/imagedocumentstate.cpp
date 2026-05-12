// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentstate.h"

#include "imagecallback.h"
#include "imagecontainer.h"
#include "imagedocumentnotifications.h"

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
    for (ImageDocumentChange change :
        imageDocumentDisplayedLocationNotifications(
            previousDisplayedUrl != displayedUrl(), previousWindowTitle != windowTitleFileName())) {
        notify(change);
    }
}

void ImageDocumentState::setStatus(ImageDocumentStatus status)
{
    if (replaceIfChanged(m_status, status)) {
        notify(ImageDocumentChange::Status);
    }
}

void ImageDocumentState::setLoading(bool loading)
{
    if (replaceIfChanged(m_loading, loading)) {
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
}
