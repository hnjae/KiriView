// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSTATE_H
#define KIRIVIEW_IMAGEDOCUMENTSTATE_H

#include "imagedocumentchangebatcher.h"
#include "imagedocumenttypes.h"
#include "location/imagelocation.h"

#include <QString>
#include <QUrl>
#include <memory>

namespace KiriView {
class ImageDocumentState
{
public:
    using ChangeCallback = ImageDocumentChangeBatcher::ChangeCallback;
    using ChangeBatch = ImageDocumentChangeBatcher::Batch;

    explicit ImageDocumentState(ChangeCallback changeCallback = {});
    explicit ImageDocumentState(ImageDocumentChangeBatcher &changes);

    ChangeBatch beginChangeBatch();

    const QUrl &sourceUrl() const;
    const DisplayedImageLocation &displayedImageLocation() const;
    const ImagePageScopeLocation &displayedImagePageScope() const;
    const QUrl &displayedUrl() const;
    ImageDocumentStatus status() const;
    bool loading() const;
    const QString &errorString() const;
    QString windowTitleFileName() const;
    const QUrl &containerNavigationUrl() const;
    const QUrl &loadingContainerNavigationUrl() const;
    bool containerNavigationAvailable() const;

    void setSourceUrl(const QUrl &sourceUrl);
    void setDisplayedImageLocation(const DisplayedImageLocation &location);
    void clearDisplayedImageLocation();
    void setStatus(ImageDocumentStatus status);
    void setLoading(bool loading);
    void setErrorString(const QString &errorString);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void setLoadingContainerNavigationUrl(const QUrl &containerUrl);
    void clearLoadingContainerNavigationUrl();

private:
    void replaceDisplayedImageLocation(DisplayedImageLocation location);
    void notify(ImageDocumentChange change);

    std::unique_ptr<ImageDocumentChangeBatcher> m_ownedChanges;
    ImageDocumentChangeBatcher *m_changes = nullptr;
    QUrl m_sourceUrl;
    DisplayedImageLocation m_displayedImageLocation;
    ImageDocumentStatus m_status = ImageDocumentStatus::Null;
    bool m_loading = false;
    QString m_errorString;
    QUrl m_containerNavigationUrl;
    QUrl m_loadingContainerNavigationUrl;
};
}

#endif
