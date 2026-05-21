// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackurlresolver.h"

#include "async/imagecallback.h"
#include "async/imageiojob.h"

#include <KIO/Job>
#include <KIO/StatJob>
#include <KJob>
#include <QObject>
#include <QString>
#include <utility>

namespace {
void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

bool isDirectBackendScheme(const QString &scheme)
{
    return scheme == QLatin1String("file") || scheme == QLatin1String("http")
        || scheme == QLatin1String("https") || scheme == QLatin1String("qrc")
        || scheme == QLatin1String("data");
}

class KioMostLocalVideoPlaybackUrlResolver final : public KiriView::VideoPlaybackUrlResolver
{
public:
    void resolve(quint64 operationId, const QUrl &sourceUrl, QObject *receiver,
        KiriView::VideoPlaybackUrlResolvedCallback resolvedCallback,
        KiriView::VideoPlaybackUrlFailedCallback failedCallback) override
    {
        cancel();
        cleanup();

        if (sourceUrl.isEmpty() || !sourceUrl.isValid()) {
            KiriView::invokeIfSet(
                failedCallback, operationId, sourceUrl, QStringLiteral("Invalid video URL."));
            return;
        }

        if (KiriView::videoPlaybackBackendCanConsumeUrl(sourceUrl)) {
            KiriView::invokeIfSet(resolvedCallback,
                KiriView::VideoPlaybackUrlResolution { operationId, sourceUrl, sourceUrl });
            return;
        }

        auto *job = KIO::mostLocalUrl(sourceUrl, KIO::HideProgressInfo);
        m_job = KiriView::ImageIoJob(job, cancelKJob);
        const KiriView::ImageIoJobCompletion completion = m_job.completion();
        QObject *context = receiver == nullptr ? job : receiver;

        QObject::connect(job, &KJob::result, context,
            [completion, job, operationId, sourceUrl,
                resolvedCallback = std::move(resolvedCallback),
                failedCallback = std::move(failedCallback)](KJob *finishedJob) mutable {
                completion.claimAndRun([&]() {
                    if (finishedJob->error() != KJob::NoError) {
                        KiriView::invokeIfSet(
                            failedCallback, operationId, sourceUrl, finishedJob->errorString());
                        return;
                    }

                    const QUrl playbackUrl = job->mostLocalUrl();
                    if (playbackUrl.isEmpty() || !playbackUrl.isValid()) {
                        KiriView::invokeIfSet(failedCallback, operationId, sourceUrl,
                            QStringLiteral("Could not resolve a local video playback URL."));
                        return;
                    }

                    KiriView::invokeIfSet(resolvedCallback,
                        KiriView::VideoPlaybackUrlResolution {
                            operationId, sourceUrl, playbackUrl });
                });
            });
    }

    void cancel() override { m_job.cancel(); }

    void cleanup() override
    {
        // This resolver uses KIOFuse/mostLocalUrl only. It does not create temporary copies.
    }

private:
    KiriView::ImageIoJob m_job;
};
}

namespace KiriView {
bool videoPlaybackBackendCanConsumeUrl(const QUrl &url)
{
    return url.isValid() && !url.isEmpty() && isDirectBackendScheme(url.scheme());
}

std::unique_ptr<VideoPlaybackUrlResolver> createDefaultVideoPlaybackUrlResolver()
{
    return std::make_unique<KioMostLocalVideoPlaybackUrlResolver>();
}
}
