// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentstate.h"

#include "imagedocumentnotifications.h"
#include "location/imagedocumentlocation.h"

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
ImageDocumentState::ImageDocumentState(ChangeCallback changeCallback)
    : m_ownedChanges(std::make_unique<ImageDocumentChangeBatcher>(std::move(changeCallback)))
    , m_changes(m_ownedChanges.get())
{
}

ImageDocumentState::ImageDocumentState(ImageDocumentChangeBatcher &changes)
    : m_changes(&changes)
{
}

ImageDocumentState::ChangeBatch ImageDocumentState::beginChangeBatch()
{
    return m_changes->beginBatch();
}

const QUrl &ImageDocumentState::sourceUrl() const { return m_sourceUrl; }

ImageNavigationCandidateKind ImageDocumentState::sourceKind() const { return m_sourceKind; }

const DisplayedImageLocation &ImageDocumentState::displayedImageLocation() const
{
    return m_displayedImageLocation;
}

const OpenedCollectionScopeLocation &ImageDocumentState::displayedOpenedCollectionScope() const
{
    return m_displayedImageLocation.openedCollectionScope();
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

bool ImageDocumentState::unsupportedOpenedCollectionVideo() const
{
    return m_unsupportedOpenedCollectionVideo;
}

void ImageDocumentState::setSourceUrl(const QUrl &sourceUrl)
{
    if (replaceIfChanged(m_sourceUrl, sourceUrl)) {
        notify(ImageDocumentChange::SourceUrl);
    }
}

void ImageDocumentState::setSourceKind(ImageNavigationCandidateKind sourceKind)
{
    m_sourceKind = sourceKind;
}

void ImageDocumentState::setDisplayedImageLocation(const DisplayedImageLocation &location)
{
    replaceDisplayedImageLocation(location);
}

void ImageDocumentState::clearDisplayedImageLocation()
{
    replaceDisplayedImageLocation(DisplayedImageLocation {});
}

void ImageDocumentState::replaceDisplayedImageLocation(DisplayedImageLocation location)
{
    const QString previousWindowTitle = windowTitleFileName();
    const QUrl previousDisplayedUrl = displayedUrl();
    if (!replaceIfChanged(m_displayedImageLocation, location)) {
        return;
    }

    m_changes->notifyAll(imageDocumentDisplayedLocationNotifications(
        previousDisplayedUrl != displayedUrl(), previousWindowTitle != windowTitleFileName()));
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

void ImageDocumentState::setUnsupportedOpenedCollectionVideo(bool unsupported)
{
    if (replaceIfChanged(m_unsupportedOpenedCollectionVideo, unsupported)) {
        notify(ImageDocumentChange::UnsupportedOpenedCollectionVideo);
    }
}

void ImageDocumentState::notify(ImageDocumentChange change) { m_changes->notify(change); }
}
