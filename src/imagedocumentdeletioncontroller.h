// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONCONTROLLER_H

#include "filedeletion.h"
#include "imagecandidaterepository.h"
#include "imagedeletioneffects.h"
#include "imagedocumenteffects.h"

#include <QString>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ImageDeletionController;
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
    void dispatch(ImageDeletionEffect effect);
    void dispatchPayload(const ClearDeletedImageAfterDeletionEffect &);
    void dispatchPayload(const OpenImageDeletionFallbackEffect &payload);
    void dispatchPayload(const OpenContainerImageDeletionFallbackEffect &payload);
    void dispatchPayload(const ReportImageDeletionFailureEffect &payload);
    void reportDocumentEffect(ImageDocumentEffect effect);

    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    Callbacks m_callbacks;
    std::unique_ptr<ImageDeletionController> m_deletionController;
};
}

#endif
