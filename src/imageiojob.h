// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEIOJOB_H
#define KIRIVIEW_IMAGEIOJOB_H

#include <QPointer>
#include <functional>
#include <memory>

class QObject;

namespace KiriView {
class ImageIoJobState final
{
public:
    using CancelCallback = std::function<void(QObject *)>;

    ImageIoJobState(QObject *object, CancelCallback cancelCallback);

    bool claim(QObject *object);
    void clear(QObject *object);
    void cancel();
    bool isActive() const;

private:
    QPointer<QObject> m_object;
    CancelCallback m_cancelCallback;
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
    bool claim(QObject *object);
    void clear(QObject *object);
    bool isActive() const;
    explicit operator bool() const;
    std::shared_ptr<ImageIoJobState> state() const;

private:
    std::shared_ptr<ImageIoJobState> m_state;
};
}

#endif
