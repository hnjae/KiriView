// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSTATE_H
#define KIRIVIEW_IMAGEDOCUMENTSTATE_H

#include "imagedocumenttypes.h"
#include "imagelocation.h"

#include <QString>
#include <QUrl>
#include <functional>
#include <vector>

namespace KiriView {
class ImageDocumentState
{
public:
    using ChangeCallback = std::function<void(ImageDocumentChange)>;

    class ChangeBatch final
    {
    public:
        explicit ChangeBatch(ImageDocumentState &state);
        ~ChangeBatch();

        ChangeBatch(const ChangeBatch &) = delete;
        ChangeBatch &operator=(const ChangeBatch &) = delete;
        ChangeBatch(ChangeBatch &&other) noexcept;
        ChangeBatch &operator=(ChangeBatch &&) = delete;

    private:
        ImageDocumentState *m_state = nullptr;
    };

    explicit ImageDocumentState(ChangeCallback changeCallback = {});

    ChangeBatch beginChangeBatch();

    const QUrl &sourceUrl() const;
    const DisplayedImageLocation &displayedImageLocation() const;
    const ArchiveDocumentLocation &displayedArchiveDocument() const;
    const QUrl &displayedUrl() const;
    const QUrl &displayedArchiveDocumentRootUrl() const;
    ImageDocumentStatus status() const;
    bool loading() const;
    const QString &errorString() const;
    QString windowTitleFileName() const;
    const QUrl &containerNavigationUrl() const;
    const QUrl &loadingContainerNavigationUrl() const;
    bool containerNavigationAvailable() const;

    void setSourceUrl(const QUrl &sourceUrl);
    void setDisplayedImageLocation(const DisplayedImageLocation &location);
    void clearDisplayedImageUrls();
    void setStatus(ImageDocumentStatus status);
    void setLoading(bool loading);
    void setErrorString(const QString &errorString);
    void setContainerNavigationUrl(const QUrl &containerUrl);
    void setLoadingContainerNavigationUrl(const QUrl &containerUrl);
    void clearLoadingContainerNavigationUrl();

private:
    void replaceDisplayedImageLocation(DisplayedImageLocation location);
    void beginBatch();
    void endBatch();
    void notify(ImageDocumentChange change);
    void emitChange(ImageDocumentChange change);

    ChangeCallback m_changeCallback;
    int m_batchDepth = 0;
    std::vector<ImageDocumentChange> m_pendingChanges;
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
