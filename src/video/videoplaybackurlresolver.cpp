// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "video/videoplaybackurlresolver.h"

#include "async/imagecallback.h"
#include "async/imageiojob.h"

#include <KProtocolInfo>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QObject>
#include <QString>
#include <utility>

namespace {
constexpr auto kioFuseService = "org.kde.KIOFuse";
constexpr auto kioFusePath = "/org/kde/KIOFuse";
constexpr auto kioFuseInterface = "org.kde.KIOFuse.VFS";

void deleteQObjectLater(QObject *object)
{
    if (object == nullptr) {
        return;
    }

    object->deleteLater();
}

bool isDirectBackendScheme(const QString &scheme)
{
    return scheme == QLatin1String("file") || scheme == QLatin1String("http")
        || scheme == QLatin1String("https") || scheme == QLatin1String("qrc")
        || scheme == QLatin1String("data");
}

bool kioProtocolMayProvideLocalPath(const QUrl &url)
{
    return KProtocolInfo::protocolClass(url.scheme()) == QLatin1String(":local");
}

class KioFuseVideoPlaybackUrlResolver final : public KiriView::VideoPlaybackUrlResolver
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

        if (!kioProtocolMayProvideLocalPath(sourceUrl)) {
            KiriView::invokeIfSet(failedCallback, operationId, sourceUrl,
                QStringLiteral("This video URL cannot be resolved to a local playback URL."));
            return;
        }

        QDBusInterface kioFuse(
            kioFuseService, kioFusePath, kioFuseInterface, QDBusConnection::sessionBus());
        auto *watcher = new QDBusPendingCallWatcher(
            kioFuse.asyncCall(QStringLiteral("mountUrl"), sourceUrl.toString(QUrl::FullyEncoded)));
        m_job = KiriView::ImageIoJob(watcher, deleteQObjectLater);
        const KiriView::ImageIoJobCompletion completion = m_job.completion();
        QObject *context = receiver == nullptr ? watcher : receiver;

        QObject::connect(watcher, &QDBusPendingCallWatcher::finished, context,
            [completion, operationId, sourceUrl, resolvedCallback = std::move(resolvedCallback),
                failedCallback = std::move(failedCallback)](
                QDBusPendingCallWatcher *finishedWatcher) mutable {
                completion.claimAndDelete([&]() {
                    const QDBusPendingReply<QString> reply(*finishedWatcher);
                    if (reply.isError()) {
                        KiriView::invokeIfSet(
                            failedCallback, operationId, sourceUrl, reply.error().message());
                        return;
                    }

                    const QUrl playbackUrl = QUrl::fromLocalFile(reply.value());
                    if (!KiriView::videoPlaybackBackendCanConsumeUrl(playbackUrl)) {
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
        // KIOFuse owns the mount lifetime. This resolver does not create temporary copies.
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
    return std::make_unique<KioFuseVideoPlaybackUrlResolver>();
}
}
