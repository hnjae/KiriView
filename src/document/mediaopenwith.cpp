// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaopenwith.h"

#include "async/imagecallback.h"

#include <KIO/ApplicationLauncherJob>
#include <KIO/Global>
#include <KIO/JobUiDelegateFactory>
#include <KJob>
#include <KJobUiDelegate>
#include <QList>
#include <QObject>
#include <utility>

namespace {
bool isUserCanceledError(int error)
{
    return error == KJob::KilledJobError || error == KIO::ERR_USER_CANCELED;
}

void cancelKJob(QObject *object)
{
    auto *job = qobject_cast<KJob *>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

KiriView::ImageIoJob startKioMediaOpenWith(QObject *receiver,
    KiriView::MediaOpenWithRequest request, KiriView::MediaOpenWithCallback callback)
{
    if (request.targetUrl.isEmpty()) {
        KiriView::invokeIfSet(callback, KiriView::MediaOpenWithResult::Failed, QString());
        return KiriView::ImageIoJob();
    }

    auto *job = new KIO::ApplicationLauncherJob(receiver);
    job->setUrls(QList<QUrl> { request.targetUrl });
    job->setUiDelegate(
        KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));

    KiriView::ImageIoJob ioJob(job, cancelKJob);
    const KiriView::ImageIoJobCompletion completion = ioJob.completion();
    QObject *context = receiver == nullptr ? job : receiver;

    QObject::connect(job, &KJob::result, context,
        [completion, callback = std::move(callback)](KJob *finishedJob) mutable {
            completion.claimAndRun([&]() {
                if (finishedJob->error() == KJob::NoError) {
                    KiriView::invokeIfSet(
                        callback, KiriView::MediaOpenWithResult::Succeeded, QString());
                    return;
                }

                if (isUserCanceledError(finishedJob->error())) {
                    KiriView::invokeIfSet(
                        callback, KiriView::MediaOpenWithResult::Canceled, QString());
                    return;
                }

                KiriView::invokeIfSet(
                    callback, KiriView::MediaOpenWithResult::Failed, finishedJob->errorString());
            });
        });
    job->start();
    return ioJob;
}
}

namespace KiriView {
MediaOpenWithProvider defaultMediaOpenWithProvider()
{
    return [](QObject *receiver, MediaOpenWithRequest request, MediaOpenWithCallback callback) {
        return startKioMediaOpenWith(receiver, std::move(request), std::move(callback));
    };
}

MediaOpenWithProvider mediaOpenWithProviderWithDefault(MediaOpenWithProvider provider)
{
    return provider ? std::move(provider) : defaultMediaOpenWithProvider();
}
}
