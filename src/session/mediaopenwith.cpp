// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mediaopenwith.h"

#include "async/imagecallback.h"

#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KJob>
#include <KJobUiDelegate>
#include <QList>
#include <QObject>
#include <utility>

namespace {
void cancelKJob(QObject* object)
{
    auto* job = qobject_cast<KJob*>(object);
    if (job == nullptr) {
        return;
    }

    job->kill(KJob::Quietly);
}

kiriview::ImageIoJob startKioMediaOpenWith(QObject* receiver,
    kiriview::MediaOpenWithRequest request, kiriview::MediaOpenWithCallback callback)
{
    if (request.targetUrl.isEmpty()) {
        kiriview::invokeIfSet(callback, kiriview::MediaOpenWithResult::Failed,
            kiriview::kioOperationValidationFailure(kiriview::KioOperationKind::MediaOpenWith,
                request.targetUrl, QStringLiteral("No Open With target is available.")));
        return kiriview::ImageIoJob();
    }

    const QUrl targetUrl = request.targetUrl;
    auto* job = new KIO::ApplicationLauncherJob(receiver);
    job->setUrls(QList<QUrl> { request.targetUrl });
    job->setUiDelegate(
        KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));

    kiriview::ImageIoJob ioJob(job, cancelKJob);
    const kiriview::ImageIoJobCompletion completion = ioJob.completion();
    QObject* context = receiver == nullptr ? job : receiver;

    QObject::connect(job, &KJob::result, context,
        [completion, targetUrl, callback = std::move(callback)](KJob* finishedJob) mutable {
            completion.claimAndRun([&]() {
                if (finishedJob->error() == KJob::NoError) {
                    kiriview::invokeIfSet(callback, kiriview::MediaOpenWithResult::Succeeded,
                        kiriview::KioOperationFailure {
                            kiriview::KioOperationKind::MediaOpenWith,
                            targetUrl,
                            std::nullopt,
                            false,
                            QString(),
                            QString(),
                            false,
                        });
                    return;
                }

                const kiriview::KioOperationFailure failure = kiriview::kioOperationFailureFromKJob(
                    kiriview::KioOperationKind::MediaOpenWith, targetUrl, finishedJob->error(),
                    finishedJob->errorString());
                if (failure.canceled) {
                    kiriview::invokeIfSet(
                        callback, kiriview::MediaOpenWithResult::Canceled, failure);
                    return;
                }

                kiriview::invokeIfSet(callback, kiriview::MediaOpenWithResult::Failed, failure);
            });
        });
    job->start();
    return ioJob;
}
}

namespace kiriview {
MediaOpenWithProvider defaultMediaOpenWithProvider()
{
    return [](QObject* receiver, MediaOpenWithRequest request, MediaOpenWithCallback callback) {
        return startKioMediaOpenWith(receiver, std::move(request), std::move(callback));
    };
}

MediaOpenWithProvider mediaOpenWithProviderWithDefault(MediaOpenWithProvider provider)
{
    return provider ? std::move(provider) : defaultMediaOpenWithProvider();
}
}
