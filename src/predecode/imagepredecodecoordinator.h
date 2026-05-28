// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "async/imageiojob.h"
#include "decoding/imagedecodedependencies.h"
#include "navigation/imagedocumentpagecandidateprovider.h"
#include "navigation/imagedocumentpagecandidaterepository.h"
#include "predecodedimage.h"
#include "predecodeloadcontroller.h"
#include "predecodescheduleruntime.h"
#include "rendering/staticimage.h"
#include "system/powersaverprovider.h"

#include <QObject>
#include <QUrl>
#include <cstddef>
#include <optional>
#include <vector>

namespace KiriView {
struct PredecodeWindowPlan;

class ImagePredecodeCoordinator final : public QObject
{
public:
    using Context = PredecodeScheduleContext;

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);
    ImagePredecodeCoordinator(QObject *parent, ImageDocumentPageCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider,
        qsizetype cacheByteBudget);

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

    ImageIoJob m_listerJob;
    ImageDocumentPageCandidateRepository m_candidateRepository;
    PredecodeLoadController m_loadController;
    PredecodeScheduleRuntime m_scheduleRuntime;
};
}

#endif
