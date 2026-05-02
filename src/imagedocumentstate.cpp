// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentstate.h"

#include <utility>

namespace KiriView {
ImageDocumentState::ImageDocumentState(ChangeCallback changeCallback)
    : m_changeCallback(std::move(changeCallback))
{
}

const QUrl &ImageDocumentState::sourceUrl() const { return m_sourceUrl; }

const QUrl &ImageDocumentState::displayedUrl() const { return m_displayedUrl; }

const QUrl &ImageDocumentState::displayedComicBookRootUrl() const
{
    return m_displayedComicBookRootUrl;
}

ImageDocumentStatus ImageDocumentState::status() const { return m_status; }

bool ImageDocumentState::loading() const { return m_loading; }

const QString &ImageDocumentState::errorString() const { return m_errorString; }

const QString &ImageDocumentState::windowTitleFileName() const { return m_windowTitleFileName; }

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

void ImageDocumentState::setDisplayedImageUrls(
    const QUrl &displayedUrl, const QUrl &comicBookRootUrl)
{
    m_displayedUrl = displayedUrl;
    m_displayedComicBookRootUrl = comicBookRootUrl;
}

void ImageDocumentState::clearDisplayedImageUrls()
{
    m_displayedUrl = QUrl();
    m_displayedComicBookRootUrl = QUrl();
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

void ImageDocumentState::setWindowTitleFileName(const QString &fileName)
{
    if (m_windowTitleFileName == fileName) {
        return;
    }

    m_windowTitleFileName = fileName;
    notify(ImageDocumentChange::WindowTitleFileName);
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
