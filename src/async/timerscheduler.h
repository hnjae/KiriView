// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_TIMERSCHEDULER_H
#define KIRIVIEW_TIMERSCHEDULER_H

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>
#include <QtGlobal>
#include <functional>
#include <memory>
#include <utility>

namespace KiriView {
class RuntimeTimerHandle
{
public:
    virtual ~RuntimeTimerHandle() = default;

    virtual void start() = 0;
    virtual void stop() = 0;
};

using RuntimeTimerCallback = std::function<void()>;
using MonotonicClock = std::function<qint64()>;
using SingleShotTimerFactory
    = std::function<std::unique_ptr<RuntimeTimerHandle>(QObject *, int, RuntimeTimerCallback)>;

struct TimerScheduler {
    MonotonicClock currentMonotonicMsec;
    SingleShotTimerFactory singleShotTimer;
};

namespace Detail {
    class QtSingleShotTimer final : public RuntimeTimerHandle
    {
    public:
        QtSingleShotTimer(QObject *owner, int intervalMsec, RuntimeTimerCallback callback)
            : m_callback(std::move(callback))
        {
            m_timer.setSingleShot(true);
            m_timer.setInterval(intervalMsec);

            QObject *context = owner == nullptr ? &m_timer : owner;
            QObject::connect(&m_timer, &QTimer::timeout, context, [this]() {
                if (m_callback) {
                    m_callback();
                }
            });
        }

        void start() override { m_timer.start(); }
        void stop() override { m_timer.stop(); }

    private:
        QTimer m_timer;
        RuntimeTimerCallback m_callback;
    };
}

inline TimerScheduler defaultTimerScheduler()
{
    auto clock = std::make_shared<QElapsedTimer>();
    clock->start();

    return TimerScheduler {
        [clock]() { return clock->isValid() ? clock->elapsed() : 0; },
        [](QObject *owner, int intervalMsec,
            RuntimeTimerCallback callback) -> std::unique_ptr<RuntimeTimerHandle> {
            return std::make_unique<Detail::QtSingleShotTimer>(
                owner, intervalMsec, std::move(callback));
        },
    };
}

inline TimerScheduler timerSchedulerWithDefaults(TimerScheduler scheduler)
{
    TimerScheduler defaults = defaultTimerScheduler();
    if (!scheduler.currentMonotonicMsec) {
        scheduler.currentMonotonicMsec = std::move(defaults.currentMonotonicMsec);
    }
    if (!scheduler.singleShotTimer) {
        scheduler.singleShotTimer = std::move(defaults.singleShotTimer);
    }

    return scheduler;
}
}

#endif
