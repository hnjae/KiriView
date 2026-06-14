// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "async/imageiojob.h"
#include "async/timerscheduler.h"
#include "decoding/imagedecodedependencies.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "navigation/imagedocumentpagecandidaterepository.h"
#include "predecodedimage.h"
#include "predecodeloadcontroller.h"
#include "predecoderuntimefacts.h"
#include "predecodescheduleruntime.h"
#include "rendering/staticimage.h"
#include "system/powersaverprovider.h"

#include <QObject>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace kiriview {
struct PredecodeWindowPlan;

class ImagePredecodeCoordinator final : public QObject
{
public:
    using Context = PredecodeScheduleContext;

    ImagePredecodeCoordinator(QObject *parent, ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider,
        qsizetype cacheByteBudget, TimerScheduler timerScheduler = {},
        PredecodeThreadCountProvider threadCountProvider = {});

    void schedule(Context context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    void cancel();
    void clear();
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

private:
    void scheduleAdjacentImagePredecode(const PredecodePendingSchedule &schedule);
    void startPredecodeImageLoads(
        const PredecodeWindowPlan &plan, const PredecodePendingSchedule &schedule);

    PredecodeThreadCountProvider m_threadCountProvider;
    ImageIoJob m_listerJob;
    ImageDocumentPageCandidateRepository m_candidateRepository;
    PredecodeLoadController m_loadController;
    PredecodeScheduleRuntime m_scheduleRuntime;
};
}

#endif
