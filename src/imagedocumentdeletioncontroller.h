// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H

#include "filedeletion.h"
#include "imagecandidaterepository.h"
#include "imagedocumentdeletionstate.h"
#include "imagedocumenteffects.h"
#include "imageiojob.h"
#include "imageremovalfallback.h"

#include <QString>
#include <QtGlobal>
#include <functional>
#include <optional>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImagePresentationController;

class ImageDocumentDeletionController final
{
public:
    using InProgressChangedCallback = std::function<void()>;
    using EffectCallback = std::function<void(ImageDocumentEffect)>;
    using FailedCallback = std::function<void(const QString &)>;

    struct Callbacks {
        InProgressChangedCallback inProgressChanged;
        EffectCallback effect;
        FailedCallback failed;
    };

    ImageDocumentDeletionController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController,
        ImageNavigationCandidateProvider candidateProvider,
        FileOperationProvider fileOperationProvider, Callbacks callbacks);
    ~ImageDocumentDeletionController();

    bool inProgress() const;
    void deleteDisplayedFile(FileDeletionMode mode);
    void cancel();

private:
    void finishFileDeletion(quint64 operationId, const ImageRemovalFallbackPlan &fallbackPlan,
        FileDeletionResult result, const QString &errorString);
    void openRemovalFallback(const ImageRemovalFallbackPlan &fallbackPlan);
    void openFallbackPlan(quint64 operationId, const NoImageRemovalFallback &);
    void openFallbackPlan(quint64 operationId, const ImageRemovalFallback &fallback);
    void openFallbackPlan(quint64 operationId, const ComicBookRemovalFallback &fallback);
    void openComicBookFallbackCandidate(quint64 operationId,
        const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate);
    void notifyInProgressChangedIf(bool changed);
    void cancelFileDeletion();
    void cancelFallback();
    void reportDocumentEffect(ImageDocumentEffect effect);
    void reportFailure(const QString &errorString);

    QObject *m_parent = nullptr;
    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    ImageCandidateRepository m_candidateRepository;
    FileOperationProvider m_fileOperationProvider;
    ImageIoJob m_fileDeletionJob;
    ImageIoJob m_fallbackJob;
    ImageDocumentDeletionState m_deletionState;
};
}

#endif
