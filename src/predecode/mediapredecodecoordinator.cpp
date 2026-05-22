// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediapredecodecoordinator.h"

#include "location/imageurl.h"
#include "predecodewindowplan.h"

#include <QThread>
#include <optional>
#include <utility>

namespace {
bool validMediaPredecodeContext(const KiriView::MediaPredecodeCoordinator::Context &context)
{
    return KiriView::normalizedValidImageUrl(context.currentUrl).has_value();
}
}

namespace KiriView {
MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent)
    : MediaPredecodeCoordinator(parent, ImageDecodeDependencies {})
{
}

MediaPredecodeCoordinator::MediaPredecodeCoordinator(QObject *parent,
    ImageDecodeDependencies decodeDependencies, PowerSaverProvider powerSaverProvider)
    : QObject(parent)
    , m_loadController(this, std::move(decodeDependencies))
{
    m_monotonicClock.start();
    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(predecodeDebounceMsec());
    m_neutralTimer.setSingleShot(true);
    m_neutralTimer.setInterval(predecodeNeutralRefreshMsec());

    QObject::connect(
        &m_debounceTimer, &QTimer::timeout, this, [this]() { startDebouncedPredecode(); });
    QObject::connect(
        &m_neutralTimer, &QTimer::timeout, this, [this]() { scheduleSettledNeutralPredecode(); });

    powerSaverProvider = powerSaverProviderWithDefault(std::move(powerSaverProvider));
    if (powerSaverProvider.monitor) {
        m_powerSaverMonitor = powerSaverProvider.monitor(
            this, [this](bool enabled) { setPowerSaverEnabled(enabled); });
    }
    if (m_powerSaverMonitor != nullptr) {
        setPowerSaverEnabled(m_powerSaverMonitor->powerSaverEnabled());
    }
}

void MediaPredecodeCoordinator::schedule(Context context)
{
    cancelBackgroundWork();
    cancelBackgroundRuntime();

    if (!validMediaPredecodeContext(context)) {
        return;
    }

    updateNavigationMomentum(context);
    const quint64 generation = m_generation.next();
    m_displayedContext = context;
    m_loadController.cacheDisplayedImages(context.displayedImages);

    if (m_powerSaverEnabled) {
        m_loadController.clearWindowUrls();
        return;
    }

    m_pendingSchedule = PendingSchedule { std::move(context), generation };
    m_debounceTimer.start();
    m_neutralTimer.start();
}

void MediaPredecodeCoordinator::setPowerSaverEnabled(bool enabled)
{
    if (m_powerSaverEnabled == enabled) {
        return;
    }

    m_powerSaverEnabled = enabled;
    if (enabled) {
        cancelBackgroundWork();
        cancelBackgroundRuntime();
        m_loadController.clearWindowUrls();
        return;
    }

    if (m_displayedContext.has_value()) {
        schedule(*m_displayedContext);
    }
}

bool MediaPredecodeCoordinator::powerSaverEnabled() const { return m_powerSaverEnabled; }

void MediaPredecodeCoordinator::startDebouncedPredecode()
{
    if (!m_pendingSchedule.has_value() || !m_generation.accepts(m_pendingSchedule->generation)) {
        return;
    }

    startPredecodeWindow(m_pendingSchedule->context, m_pendingSchedule->generation);
}

void MediaPredecodeCoordinator::scheduleSettledNeutralPredecode()
{
    if (!m_pendingSchedule.has_value() || m_momentumState.mode == PredecodeMomentumMode::Neutral) {
        return;
    }

    m_momentumState.mode = PredecodeMomentumMode::Neutral;
    m_pendingSchedule->generation = m_generation.next();
    cancelBackgroundRuntime();
    startPredecodeWindow(m_pendingSchedule->context, m_pendingSchedule->generation);
}

void MediaPredecodeCoordinator::startPredecodeWindow(const Context &context, quint64 generation)
{
    if (!m_generation.accepts(generation)) {
        return;
    }

    const PredecodeWindowPlan plan = predecodeWindowPlanForMediaCandidates(
        context.currentUrl, context.candidates, policyInput());
    m_loadController.startWindowLoads(PredecodeLoadWindow {
        context.currentUrl,
        plan.archiveDocument,
        plan.urls,
        context.displayedImages,
        context.firstDisplayContext,
        generation,
        plan.parallelLimit,
    });
}

void MediaPredecodeCoordinator::cancelBackgroundWork()
{
    m_generation.invalidate();
    m_pendingSchedule.reset();
}

void MediaPredecodeCoordinator::cancelBackgroundRuntime()
{
    m_debounceTimer.stop();
    m_neutralTimer.stop();
    m_loadController.cancelBackgroundWork();
}

void MediaPredecodeCoordinator::updateNavigationMomentum(const Context &context)
{
    const std::optional<std::size_t> currentIndex
        = mediaNavigationCandidateIndex(context.candidates, context.currentUrl);
    m_momentumState = predecodeUpdatedMomentumState(m_momentumState,
        currentIndex.has_value() ? static_cast<int>(*currentIndex) : -1, currentMonotonicMsec());
}

PredecodePolicyInput MediaPredecodeCoordinator::policyInput() const
{
    return PredecodePolicyInput {
        PredecodeDocumentKind::Regular,
        m_momentumState.mode,
        m_powerSaverEnabled,
        QThread::idealThreadCount(),
    };
}

qint64 MediaPredecodeCoordinator::currentMonotonicMsec() const
{
    return m_monotonicClock.isValid() ? m_monotonicClock.elapsed() : 0;
}

void MediaPredecodeCoordinator::cancel()
{
    cancelBackgroundWork();
    cancelBackgroundRuntime();
    m_displayedContext.reset();
    m_momentumState = {};
}

void MediaPredecodeCoordinator::clear()
{
    cancel();
    m_loadController.clear();
}

std::optional<PredecodedImage> MediaPredecodeCoordinator::findPredecodedImage(const QUrl &url) const
{
    return m_loadController.findPredecodedImage(url);
}
}
