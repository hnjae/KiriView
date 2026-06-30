// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videosourceloadruntime.h"

#include <utility>

namespace {
void dispatchPlan(
    const kiriview::VideoSourceLoadPlanCallback& callback, kiriview::VideoSourceLoadPlan plan)
{
    if (callback) {
        callback(std::move(plan));
    }
}
}

namespace kiriview {
VideoSourceLoadRuntime::VideoSourceLoadRuntime(std::unique_ptr<VideoPlaybackUrlResolver> resolver)
    : m_resolver(
          resolver == nullptr ? createDefaultVideoPlaybackUrlResolver() : std::move(resolver))
{
}

VideoSourceLoadRuntime::~VideoSourceLoadRuntime() { shutdown(); }

bool VideoSourceLoadRuntime::active() const { return m_resolution.active(); }

void VideoSourceLoadRuntime::setSourceUrl(
    const QUrl& sourceUrl, QObject* receiver, VideoSourceLoadPlanCallback planCallback)
{
    m_shutdown = false;
    cancelAndCleanup();

    if (sourceUrl.isEmpty()) {
        dispatchPlan(planCallback, videoSourceLoadClearPlan());
        return;
    }

    const ImageAsyncScopedOperation<QUrl> operation = m_resolution.start(sourceUrl);
    dispatchPlan(planCallback, videoSourceLoadStartPlan(sourceUrl));

    m_resolver->resolve(
        operation.operationId, sourceUrl, receiver,
        [this, planCallback](VideoPlaybackUrlResolution resolution) {
            completePlaybackUrlResolution(resolution, planCallback);
        },
        [this, planCallback](quint64 operationId, QUrl failedSourceUrl, QString errorString) {
            failPlaybackUrlResolution(operationId, failedSourceUrl, errorString, planCallback);
        });
}

void VideoSourceLoadRuntime::cancelPendingResolution()
{
    m_shutdown = false;
    cancelAndCleanup();
}

void VideoSourceLoadRuntime::shutdown()
{
    if (m_shutdown) {
        return;
    }

    cancelAndCleanup();
    m_shutdown = true;
}

void VideoSourceLoadRuntime::cancelAndCleanup()
{
    m_resolution.cancel();
    if (m_resolver != nullptr) {
        m_resolver->cancel();
        m_resolver->cleanup();
    }
}

void VideoSourceLoadRuntime::completePlaybackUrlResolution(
    const VideoPlaybackUrlResolution& resolution, const VideoSourceLoadPlanCallback& callback)
{
    if (!m_resolution.finish(resolution.operationId, resolution.sourceUrl)) {
        return;
    }

    dispatchPlan(callback, videoSourceLoadReadyPlan(resolution.sourceUrl, resolution.playbackUrl));
}

void VideoSourceLoadRuntime::failPlaybackUrlResolution(quint64 operationId, const QUrl& sourceUrl,
    const QString& errorString, const VideoSourceLoadPlanCallback& callback)
{
    if (!m_resolution.finish(operationId, sourceUrl)) {
        return;
    }

    dispatchPlan(callback, videoSourceLoadFailurePlan(sourceUrl, errorString));
}
}
