// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEPREDECODECOORDINATOR_H
#define KIRIVIEW_IMAGEPREDECODECOORDINATOR_H

#include "decoding/imagedecodedependencies.h"
#include "imageiojob.h"
#include "navigation/imagecandidateprovider.h"
#include "navigation/imagecandidaterepository.h"
#include "powersaverprovider.h"
#include "predecode/predecodedimage.h"
#include "predecode/predecodeloadcontroller.h"
#include "predecode/predecodeschedulestate.h"
#include "rendering/staticimage.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace KiriView {
class ImagePredecodeCoordinator final : public QObject
{
public:
    using Context = PredecodeScheduleContext;

    explicit ImagePredecodeCoordinator(QObject *parent = nullptr);
    ImagePredecodeCoordinator(QObject *parent, ImageNavigationCandidateProvider candidateProvider,
        ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider = {});

    void schedule(Context context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    void cancel();
    void clear();
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

private:
    void executeScheduleEffects(const PredecodeScheduleEffectPlan &plan);
    void cacheDisplayedImages(const Context &context);
    std::vector<DisplayedPredecodeImage> displayedImages(const Context &context) const;
    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void scheduleAdjacentImagePredecode(const Context &context, quint64 generation);
    void startPredecodeImageLoads(const std::vector<QUrl> &urls,
        const ArchiveDocumentLocation &archiveDocument, const Context &context, quint64 generation,
        std::size_t parallelLimit);
    void cancelBackgroundWork();
    void cancelBackgroundEffects();
    qint64 currentMonotonicMsec() const;

    ImageIoJob m_listerJob;
    ImageCandidateRepository m_candidateRepository;
    PredecodeLoadController m_loadController;
    std::unique_ptr<PowerSaverStateMonitor> m_powerSaverMonitor;
    PredecodeScheduleState m_scheduleState;
    QTimer m_debounceTimer;
    QTimer m_neutralTimer;
    QElapsedTimer m_monotonicClock;
};
}

#endif
