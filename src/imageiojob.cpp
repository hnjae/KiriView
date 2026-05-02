// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageiojob.h"

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
    if (m_object == nullptr || object != m_object) {
        return false;
    }

    m_object = nullptr;
    m_cancelCallback = {};
    return true;
}

void ImageIoJobState::clear(QObject *object)
{
    if (object != m_object) {
        return;
    }

    m_object = nullptr;
    m_cancelCallback = {};
}

void ImageIoJobState::cancel()
{
    if (m_object == nullptr) {
        return;
    }

    QObject *object = m_object;
    CancelCallback cancelCallback = std::move(m_cancelCallback);
    m_object = nullptr;
    if (cancelCallback) {
        cancelCallback(object);
    }
}

bool ImageIoJobState::isActive() const { return m_object != nullptr; }

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

bool ImageIoJob::claim(QObject *object) { return m_state != nullptr && m_state->claim(object); }

void ImageIoJob::clear(QObject *object)
{
    if (m_state != nullptr) {
        m_state->clear(object);
    }
}

bool ImageIoJob::isActive() const { return m_state != nullptr && m_state->isActive(); }

ImageIoJob::operator bool() const { return isActive(); }

std::shared_ptr<ImageIoJobState> ImageIoJob::state() const { return m_state; }
}
