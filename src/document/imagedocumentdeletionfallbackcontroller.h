// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTDELETIONFALLBACKCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTDELETIONFALLBACKCONTROLLER_H

#include "imageasyncoperationstate.h"
#include "imagecandidaterepository.h"
#include "imagedocumenteffects.h"
#include "imageiojob.h"
#include "imageremovalfallback.h"

#include <QtGlobal>
#include <functional>
#include <optional>

class QObject;

namespace KiriView {
class ImageDocumentDeletionFallbackController final
{
public:
    using EffectCallback = std::function<void(ImageDocumentEffect)>;

    ImageDocumentDeletionFallbackController(QObject *parent,
        ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback);
    ~ImageDocumentDeletionFallbackController();

    void open(const ImageRemovalFallbackPlan &fallbackPlan);
    void cancel();

private:
    void openFallbackPlan(quint64 operationId, const NoImageRemovalFallback &);
    void openFallbackPlan(quint64 operationId, const ImageRemovalFallback &fallback);
    void openFallbackPlan(quint64 operationId, const ComicBookRemovalFallback &fallback);
    void openComicBookFallbackCandidate(quint64 operationId,
        const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate);
    void reportDocumentEffect(ImageDocumentEffect effect);

    QObject *m_parent = nullptr;
    ImageCandidateRepository m_candidateRepository;
    EffectCallback m_effectCallback;
    ImageIoJob m_job;
    ImageAsyncOperationState m_operation;
};
}

#endif
