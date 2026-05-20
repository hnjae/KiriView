// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCWORKER_H
#define KIRIVIEW_IMAGEASYNCWORKER_H

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QRunnable>
#include <QThreadPool>
#include <Qt>
#include <memory>
#include <type_traits>
#include <utility>

namespace KiriView {
namespace Detail {
    template <typename Work, typename Finish> struct AsyncWorkerCallbacks {
        Work work;
        Finish finish;
    };
}

template <typename Work, typename Finish>
void runAsyncWorker(QObject *context, Work work, Finish finish)
{
    using Result = std::invoke_result_t<Work &>;
    static_assert(!std::is_void_v<Result>, "runAsyncWorker requires a non-void work result");

    auto callbacks = std::make_shared<Detail::AsyncWorkerCallbacks<Work, Finish>>(
        Detail::AsyncWorkerCallbacks<Work, Finish> { std::move(work), std::move(finish) });

    if (context == nullptr) {
        callbacks->finish(callbacks->work());
        return;
    }

    const QPointer<QObject> guardedContext(context);
    QThreadPool::globalInstance()->start(QRunnable::create([guardedContext, callbacks]() mutable {
        Result result = callbacks->work();
        if (guardedContext == nullptr) {
            return;
        }

        QMetaObject::invokeMethod(
            guardedContext.data(),
            [guardedContext, callbacks, result = std::move(result)]() mutable {
                if (guardedContext == nullptr) {
                    return;
                }

                callbacks->finish(std::move(result));
            },
            Qt::QueuedConnection);
    }));
}
}

#endif
