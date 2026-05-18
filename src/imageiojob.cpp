// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageiojob.h"

#include "imagecallback.h"

#include <QObject>
#include <utility>

namespace KiriView {
ImageIoJobState::ImageIoJobState(QObject *object, CancelCallback cancelCallback)
    : m_object(object)
    , m_cancelCallback(std::move(cancelCallback))
{
}

bool ImageIoJobState::claim(QObject *object)
{
    if (m_object.isNull() || object != m_object.data()) {
        return false;
    }

    m_object.clear();
    m_cancelCallback = {};
    return true;
}

void ImageIoJobState::cancel()
{
    if (m_object.isNull()) {
        m_cancelCallback = {};
        return;
    }

    QObject *object = m_object.data();
    CancelCallback cancelCallback = std::move(m_cancelCallback);
    m_object.clear();
    invokeIfSet(cancelCallback, object);
}

bool ImageIoJobState::isActive() const { return !m_object.isNull(); }

QObject *ImageIoJobState::object() const { return m_object.data(); }

ImageIoJobCompletion::ImageIoJobCompletion(QObject *object, std::shared_ptr<ImageIoJobState> state)
    : m_object(object)
    , m_state(std::move(state))
{
}

QObject *ImageIoJobCompletion::object() const { return m_object.data(); }

bool ImageIoJobCompletion::isActive() const
{
    return m_state != nullptr && m_state->isActive() && !m_object.isNull();
}

void ImageIoJobCompletion::cancel() const
{
    if (m_state == nullptr) {
        return;
    }

    m_state->cancel();
}

ImageIoJob::ImageIoJob(QObject *object, CancelCallback cancelCallback)
    : m_state(std::make_shared<ImageIoJobState>(object, std::move(cancelCallback)))
{
}

ImageIoJob::~ImageIoJob() { cancel(); }

ImageIoJob &ImageIoJob::operator=(ImageIoJob &&other) noexcept
{
    if (this == &other) {
        return *this;
    }

    cancel();
    m_state = std::move(other.m_state);
    return *this;
}

void ImageIoJob::cancel()
{
    if (m_state == nullptr) {
        return;
    }

    m_state->cancel();
}

bool ImageIoJob::isActive() const { return m_state != nullptr && m_state->isActive(); }

ImageIoJobCompletion ImageIoJob::completion() const
{
    return ImageIoJobCompletion(m_state == nullptr ? nullptr : m_state->object(), m_state);
}
}
