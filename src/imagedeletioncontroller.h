// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDELETIONCONTROLLER_H

#include "filedeletion.h"
#include "filedeletionfallback.h"
#include "imagecandidaterepository.h"
#include "imageiojob.h"
#include "imagelocation.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>

namespace KiriView {
class ImageDeletionController final : public QObject
{
public:
    using InProgressChangedCallback = std::function<void()>;
    using ClearDeletedImageCallback = std::function<void()>;
    using OpenUrlCallback = std::function<void(const QUrl &)>;
    using OpenContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;
    using FailedCallback = std::function<void(const QString &)>;

    struct Callbacks {
        InProgressChangedCallback inProgressChanged;
        ClearDeletedImageCallback clearDeletedImage;
        OpenUrlCallback openUrl;
        OpenContainerImageCallback openContainerImage;
        FailedCallback failed;
    };

    ImageDeletionController(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        FileOperationProvider fileOperationProvider, Callbacks callbacks);
    ~ImageDeletionController() override;

    bool inProgress() const;
    void deleteDisplayedFile(const DisplayedImageLocation &location, FileDeletionMode mode);
    void cancel();

private:
    void finishFileDeletion(const DeletionFallbackPlan &fallbackPlan, FileDeletionResult result,
        const QString &errorString);
    void openDeletionFallback(const DeletionFallbackPlan &fallbackPlan);
    void openDeletionFallbackPlan(const NoDeletionFallbackPlan &fallbackPlan);
    void openDeletionFallbackPlan(const ImageDeletionFallbackPlan &fallbackPlan);
    void openDeletionFallbackPlan(const ComicBookDeletionFallbackPlan &fallbackPlan);
    void openComicBookDeletionFallbackCandidate(
        const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate);
    void setInProgress(bool inProgress);
    void cancelFileDeletion();
    void cancelFallback();
    void reportFailure(const QString &errorString);

    Callbacks m_callbacks;
    ImageCandidateRepository m_candidateRepository;
    FileOperationProvider m_fileOperationProvider;
    ImageIoJob m_fileDeletionJob;
    ImageIoJob m_fallbackJob;
    bool m_inProgress = false;
};
}

#endif
