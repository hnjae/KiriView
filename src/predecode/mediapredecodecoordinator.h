// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODECOORDINATOR_H
#define KIRIVIEW_MEDIAPREDECODECOORDINATOR_H

#include "decoding/imagedecodedependencies.h"
#include "navigation/medianavigationmodel.h"
#include "predecodedimage.h"
#include "predecodeloadcontroller.h"
#include "predecodeschedulestate.h"
#include "system/powersaverprovider.h"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QUrl>
#include <memory>
#include <optional>
#include <vector>

namespace KiriView {
class MediaPredecodeCoordinator final : public QObject
{
public:
    struct Context {
        QUrl currentUrl;
        std::vector<MediaNavigationCandidate> candidates;
        std::vector<DisplayedPredecodeImage> displayedImages;
        ImageFirstDisplayDecodeContext firstDisplayContext;
    };

    explicit MediaPredecodeCoordinator(QObject *parent = nullptr);
    MediaPredecodeCoordinator(QObject *parent, ImageDecodeDependencies decodeDependencies,
        PowerSaverProvider powerSaverProvider = {});

    void schedule(Context context);
    void setPowerSaverEnabled(bool enabled);
    bool powerSaverEnabled() const;
    void cancel();
    void clear();
    std::optional<PredecodedImage> findPredecodedImage(const QUrl &url) const;

private:
    void dispatchSchedulePlan(const PredecodeScheduleRuntimePlan &plan);
    void dispatchScheduleOperation(const PredecodeScheduleOperation &operation);
    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void startPredecodeWindow(const PredecodePendingSchedule &schedule);
    void cancelBackgroundRuntime();
    PredecodeScheduleContext scheduleContext(const Context &context) const;
    PredecodePolicyInput policyInput() const;
    qint64 currentMonotonicMsec() const;

    PredecodeLoadController m_loadController;
    std::unique_ptr<PowerSaverStateMonitor> m_powerSaverMonitor;
    PredecodeScheduleState m_scheduleState;
    std::vector<MediaNavigationCandidate> m_currentCandidates;
    QTimer m_debounceTimer;
    QTimer m_neutralTimer;
    QElapsedTimer m_monotonicClock;
};
}

#endif
