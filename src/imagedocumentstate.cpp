// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentstate.h"

#include "imagecontainer.h"

#include <utility>

namespace KiriView {
ImageDocumentState::ImageDocumentState(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
}

const QUrl &ImageDocumentState::sourceUrl() const { return m_sourceUrl; }

const DisplayedImageLocation &ImageDocumentState::displayedImageLocation() const
{
    return m_displayedImageLocation;
}

const QUrl &ImageDocumentState::displayedUrl() const { return m_displayedImageLocation.imageUrl(); }

const QUrl &ImageDocumentState::displayedComicBookRootUrl() const
{
    return m_displayedImageLocation.comicBookRootUrl();
}

ImageDocumentStatus ImageDocumentState::status() const { return m_status; }

bool ImageDocumentState::loading() const { return m_loading; }

const QString &ImageDocumentState::errorString() const { return m_errorString; }

QString ImageDocumentState::windowTitleFileName() const
{
    return windowTitleFileNameForDisplayedUrl(displayedUrl(), displayedComicBookRootUrl());
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
    if (m_sourceUrl == sourceUrl) {
        return;
    }

    m_sourceUrl = sourceUrl;
    notify(ImageDocumentChange::SourceUrl);
}

void ImageDocumentState::setDisplayedImageLocation(const DisplayedImageLocation &location)
{
    const QString previousWindowTitle = windowTitleFileName();
    const QUrl previousDisplayedUrl = displayedUrl();
    m_displayedImageLocation = location;
    if (previousDisplayedUrl != displayedUrl()) {
        notify(ImageDocumentChange::DisplayedUrl);
    }
    if (previousWindowTitle != windowTitleFileName()) {
        notify(ImageDocumentChange::WindowTitleFileName);
    }
}

void ImageDocumentState::clearDisplayedImageUrls()
{
    const QString previousWindowTitle = windowTitleFileName();
    const QUrl previousDisplayedUrl = displayedUrl();
    m_displayedImageLocation = {};
    if (previousDisplayedUrl != displayedUrl()) {
        notify(ImageDocumentChange::DisplayedUrl);
    }
    if (previousWindowTitle != windowTitleFileName()) {
        notify(ImageDocumentChange::WindowTitleFileName);
    }
}

void ImageDocumentState::setStatus(ImageDocumentStatus status)
{
    if (m_status == status) {
        return;
    }

    m_status = status;
    notify(ImageDocumentChange::Status);
}

void ImageDocumentState::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }

    m_loading = loading;
    notify(ImageDocumentChange::Loading);
}

void ImageDocumentState::setErrorString(const QString &errorString)
{
    if (m_errorString == errorString) {
        return;
    }

    m_errorString = errorString;
    notify(ImageDocumentChange::ErrorString);
}

void ImageDocumentState::setContainerNavigationUrl(const QUrl &containerUrl)
{
    if (m_containerNavigationUrl == containerUrl) {
        return;
    }

    m_containerNavigationUrl = containerUrl;
    notify(ImageDocumentChange::ContainerNavigation);
}

void ImageDocumentState::setLoadingContainerNavigationUrl(const QUrl &containerUrl)
{
    m_loadingContainerNavigationUrl = containerUrl;
}

void ImageDocumentState::clearLoadingContainerNavigationUrl()
{
    m_loadingContainerNavigationUrl = QUrl();
}

void ImageDocumentState::notify(ImageDocumentChange change)
{
    if (m_changeCallback) {
        m_changeCallback(change);
    }
}
}
