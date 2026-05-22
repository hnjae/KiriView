// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_MEDIAPREDECODECOORDINATOR_H
#define KIRIVIEW_MEDIAPREDECODECOORDINATOR_H

#include "async/imageasyncticket.h"
#include "decoding/imagedecodedependencies.h"
#include "navigation/medianavigationmodel.h"
#include "predecodedimage.h"
#include "predecodeloadcontroller.h"
#include "predecodepolicy.h"
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
    struct PendingSchedule {
        Context context;
        quint64 generation = 0;
    };

    void startDebouncedPredecode();
    void scheduleSettledNeutralPredecode();
    void startPredecodeWindow(const Context &context, quint64 generation);
    void cancelBackgroundWork();
    void cancelBackgroundRuntime();
    void updateNavigationMomentum(const Context &context);
    PredecodePolicyInput policyInput() const;
    qint64 currentMonotonicMsec() const;

    PredecodeLoadController m_loadController;
    std::unique_ptr<PowerSaverStateMonitor> m_powerSaverMonitor;
    std::optional<Context> m_displayedContext;
    std::optional<PendingSchedule> m_pendingSchedule;
    ImageAsyncTicket m_generation;
    PredecodeMomentumState m_momentumState;
    QTimer m_debounceTimer;
    QTimer m_neutralTimer;
    QElapsedTimer m_monotonicClock;
    bool m_powerSaverEnabled = false;
};
}

#endif
