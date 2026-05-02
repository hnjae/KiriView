// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSTATE_H
#define KIRIVIEW_IMAGEDOCUMENTSTATE_H

#include "imagedocumenttypes.h"
#include "imagelocation.h"

#include <QString>
#include <QUrl>
#include <functional>

namespace KiriView {
class ImageDocumentState
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;

    explicit ImageDocumentState(ChangeCallback changeCallback = {});

    const QUrl &sourceUrl() const;
    const DisplayedImageLocation &displayedImageLocation() const;
    const QUrl &displayedUrl() const;
    const QUrl &displayedComicBookRootUrl() const;
    ImageDocumentStatus status() const;
    bool loading() const;
    const QString &errorString() const;
    const QString &windowTitleFileName() const;
    const QUrl &containerNavigationUrl() const;
    const QUrl &loadingContainerNavigationUrl() const;
    bool containerNavigationAvailable() const;

    void setSourceUrl(const QUrl &sourceUrl);
    void setDisplayedImageLocation(const DisplayedImageLocation &location);
    void clearDisplayedImageUrls();
    void setStatus(ImageDocumentStatus status);
    void setLoading(bool loading);
    void setErrorString(const QString &errorString);
    void setWindowTitleFileName(const QString &fileName);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void setLoadingContainerNavigationUrl(const QUrl &containerUrl);
    void clearLoadingContainerNavigationUrl();

private:
    void notify(ImageDocumentChange change);

    ChangeCallback m_changeCallback;
    QUrl m_sourceUrl;
    DisplayedImageLocation m_displayedImageLocation;
    ImageDocumentStatus m_status = ImageDocumentStatus::Null;
    bool m_loading = false;
    QString m_errorString;
    QString m_windowTitleFileName;
    QUrl m_containerNavigationUrl;
    QUrl m_loadingContainerNavigationUrl;
};
}

#endif
