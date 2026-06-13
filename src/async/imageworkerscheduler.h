// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEWORKERSCHEDULER_H
#define KIRIVIEW_IMAGEWORKERSCHEDULER_H

#include "async/imageasyncworker.h"

#include <QObject>
#include <QPointer>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace kiriview {
using ImageWorkerOperation = std::function<void()>;
using ImageWorkerCompletion = std::function<void()>;
using ImageWorkerScheduleCallback
    = std::function<void(QObject *, ImageWorkerOperation, ImageWorkerCompletion)>;

inline void defaultImageWorkerSchedule(
    QObject *context, ImageWorkerOperation operation, ImageWorkerCompletion completion)
{
    runAsyncWorker(
        context,
        [operation = std::move(operation)]() mutable {
            operation();
            return true;
        },
        [completion = std::move(completion)](bool) mutable {
            if (completion) {
                completion();
            }
        });
}

class ImageWorkerScheduler final
{
public:
    ImageWorkerScheduler() = default;

    explicit ImageWorkerScheduler(ImageWorkerScheduleCallback schedule)
        : m_schedule(std::move(schedule))
    {
    }

    bool isValid() const { return static_cast<bool>(m_schedule); }

    template <typename Work, typename Finish>
    void run(QObject *context, Work work, Finish finish) const
    {
        using Result = std::invoke_result_t<Work &>;
        static_assert(
            !std::is_void_v<Result>, "ImageWorkerScheduler requires a non-void work result");

        const bool guardContext = context != nullptr;
        const QPointer<QObject> guardedContext(context);
        auto result = std::make_shared<std::optional<Result>>();
        ImageWorkerOperation operation
            = [work = std::move(work), result]() mutable { result->emplace(work()); };
        ImageWorkerCompletion completion
            = [guardContext, guardedContext, finish = std::move(finish), result]() mutable {
                  if (guardContext && guardedContext == nullptr) {
                      return;
                  }
                  if (!result->has_value()) {
                      return;
                  }

                  finish(std::move(**result));
                  result->reset();
              };

        schedule(context, std::move(operation), std::move(completion));
    }

private:
    void schedule(
        QObject *context, ImageWorkerOperation operation, ImageWorkerCompletion completion) const
    {
        if (m_schedule) {
            m_schedule(context, std::move(operation), std::move(completion));
            return;
        }

        defaultImageWorkerSchedule(context, std::move(operation), std::move(completion));
    }

    ImageWorkerScheduleCallback m_schedule;
};

inline ImageWorkerScheduler defaultImageWorkerScheduler()
{
    return ImageWorkerScheduler(defaultImageWorkerSchedule);
}
}

#endif
