// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEIOWORKERJOB_H
#define KIRIVIEW_IMAGEIOWORKERJOB_H

#include "async/imageasyncworker.h"
#include "async/imageiojob.h"

#include <QObject>
#include <utility>

namespace KiriView {
namespace Detail {
    inline void cancelImageIoWorkerToken(QObject *object)
    {
        if (object != nullptr) {
            object->deleteLater();
        }
    }
}

template <typename Work, typename Finish>
ImageIoJob startImageIoWorkerJob(QObject *context, QObject *receiver, Work work, Finish finish)
{
    if (context == nullptr || receiver == nullptr) {
        std::forward<Finish>(finish)(std::forward<Work>(work)());
        return ImageIoJob();
    }

    auto *token = new QObject(receiver);
    ImageIoJob ioJob(token, Detail::cancelImageIoWorkerToken);
    const ImageIoJobCompletion completion = ioJob.completion();

    runAsyncWorker(context, std::forward<Work>(work),
        [completion, finish = std::forward<Finish>(finish)](auto result) mutable {
            completion.claimAndDelete([&]() mutable { finish(std::move(result)); });
        });
    return ioJob;
}

template <typename Work, typename Finish>
ImageIoJob startImageIoWorkerJob(QObject *receiver, Work work, Finish finish)
{
    return startImageIoWorkerJob(
        receiver, receiver, std::forward<Work>(work), std::forward<Finish>(finish));
}
}

#endif
