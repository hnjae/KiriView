// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEIOJOB_H
#define KIRIVIEW_IMAGEIOJOB_H

#include <QObject>
#include <QPointer>
#include <functional>
#include <memory>
#include <utility>

namespace KiriView {
class ImageIoJobState final
{
public:
    using CancelCallback = std::function<void(QObject *)>;

    ImageIoJobState(QObject *object, CancelCallback cancelCallback);

    bool claim(QObject *object);
    template <typename Finish> bool claimAndRun(QObject *object, Finish &&finish)
    {
        if (!claim(object)) {
            return false;
        }

        std::forward<Finish>(finish)();
        return true;
    }
    void cancel();
    bool isActive() const;

private:
    friend class ImageIoJob;
    friend class ImageIoJobCompletion;

    QObject *token() const;

    QPointer<QObject> m_token;
    QPointer<QObject> m_activeObject;
    CancelCallback m_cancelCallback;
};

class ImageIoJobCompletion final
{
public:
    ImageIoJobCompletion() = default;
    explicit ImageIoJobCompletion(std::shared_ptr<ImageIoJobState> state);

    QObject *object() const;
    bool isActive() const;
    void cancel() const;

    template <typename Finish> bool claimAndRun(Finish &&finish) const
    {
        QObject *object = this->object();
        if (m_state == nullptr || object == nullptr) {
            return false;
        }

        return m_state->claimAndRun(object, std::forward<Finish>(finish));
    }

    template <typename Finish> bool claimAndDelete(Finish &&finish) const
    {
        QObject *object = this->object();
        if (m_state == nullptr || object == nullptr) {
            return false;
        }

        return m_state->claimAndRun(object, [&]() {
            std::forward<Finish>(finish)();
            object->deleteLater();
        });
    }

private:
    std::shared_ptr<ImageIoJobState> m_state;
};

class ImageIoJob final
{
public:
    using CancelCallback = ImageIoJobState::CancelCallback;

    ImageIoJob() = default;
    ImageIoJob(QObject *object, CancelCallback cancelCallback);
    ~ImageIoJob();

    ImageIoJob(const ImageIoJob &) = delete;
    ImageIoJob &operator=(const ImageIoJob &) = delete;
    ImageIoJob(ImageIoJob &&) noexcept = default;
    ImageIoJob &operator=(ImageIoJob &&other) noexcept;

    void cancel();
    bool isActive() const;
    ImageIoJobCompletion completion() const;

private:
    std::shared_ptr<ImageIoJobState> m_state;
};
}

#endif
