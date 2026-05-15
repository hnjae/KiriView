// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDELETIONFALLBACKEXECUTOR_H
#define KIRIVIEW_IMAGEDELETIONFALLBACKEXECUTOR_H

#include "filedeletionfallback.h"
#include "imagecandidaterepository.h"
#include "imagedeletioneffects.h"
#include "imageiojob.h"
#include "imagenavigationtypes.h"

#include <QObject>
#include <functional>
#include <optional>

namespace KiriView {
class ImageDeletionFallbackExecutor final
{
public:
    using EffectCallback = std::function<void(ImageDeletionEffect)>;

    ImageDeletionFallbackExecutor(QObject *receiver,
        ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback);

    void open(const DeletionFallbackPlan &fallbackPlan);
    void cancel();

private:
    void openPlan(const NoDeletionFallbackPlan &);
    void openPlan(const ImageDeletionFallbackPlan &fallbackPlan);
    void openPlan(const ComicBookDeletionFallbackPlan &fallbackPlan);
    void openComicBookCandidate(const std::optional<ContainerNavigationCandidate> &candidate,
        const std::optional<ContainerNavigationCandidate> &fallbackCandidate);
    void report(ImageDeletionEffect effect);

    QObject *m_receiver = nullptr;
    ImageCandidateRepository m_candidateRepository;
    EffectCallback m_effectCallback;
    ImageIoJob m_job;
};
}

#endif
