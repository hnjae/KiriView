// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEREMOVALFALLBACKEXECUTOR_H
#define KIRIVIEW_IMAGEREMOVALFALLBACKEXECUTOR_H

#include "imagecandidaterepository.h"
#include "imagedeletioneffects.h"
#include "imageiojob.h"
#include "imagenavigationtypes.h"
#include "imageremovalfallback.h"

#include <QObject>
#include <functional>
#include <optional>

namespace KiriView {
class ImageRemovalFallbackExecutor final
{
public:
    using EffectCallback = std::function<void(ImageDeletionEffect)>;

    ImageRemovalFallbackExecutor(QObject *receiver,
        ImageNavigationCandidateProvider candidateProvider, EffectCallback effectCallback);

    void open(const ImageRemovalFallbackPlan &fallbackPlan);
    void cancel();

private:
    void openPlan(const NoImageRemovalFallback &);
    void openPlan(const ImageRemovalFallback &fallback);
    void openPlan(const ComicBookRemovalFallback &fallback);
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
