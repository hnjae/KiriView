// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H

#include "async/imageiojob.h"
#include "filedeletion.h"
#include "imagedocumentdeletionfallbackcontroller.h"
#include "imagedocumentdeletionstate.h"
#include "imagedocumentruntimeplan.h"
#include "imageremovalfallback.h"
#include "navigation/imagedocumentpagecandidaterepository.h"

#include <QString>
#include <QtGlobal>
#include <functional>

class QObject;

namespace KiriView {
class ImageDocumentState;
class ImagePresentationController;

class ImageDocumentDeletionController final
{
public:
    using InProgressChangedCallback = std::function<void()>;
    using RuntimePlanCallback = std::function<void(ImageDocumentRuntimePlan)>;
    using FailedCallback = std::function<void(const QString &)>;

    struct Callbacks {
        InProgressChangedCallback inProgressChanged;
        RuntimePlanCallback runtimePlan;
        FailedCallback failed;
    };

    ImageDocumentDeletionController(QObject *parent, ImageDocumentState &state,
        ImagePresentationController &presentationController,
        ImageDocumentPageCandidateProvider candidateProvider,
        FileOperationProvider fileOperationProvider, Callbacks callbacks);
    ~ImageDocumentDeletionController();

    bool inProgress() const;
    void deleteDisplayedFile(FileDeletionMode mode);
    void cancel();

private:
    void finishFileDeletion(quint64 operationId, const ImageRemovalFallbackPlan &fallbackPlan,
        FileDeletionResult result, const QString &errorString);
    void notifyInProgressChangedIf(bool changed);
    void cancelFileDeletion();
    void reportRuntimePlan(ImageDocumentRuntimePlan plan);
    void reportFailure(const QString &errorString);

    QObject *m_parent = nullptr;
    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    FileOperationProvider m_fileOperationProvider;
    ImageIoJob m_fileDeletionJob;
    ImageDocumentDeletionState m_deletionState;
    ImageDocumentDeletionFallbackController m_fallbackController;
};
}

#endif
