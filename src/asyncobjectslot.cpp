// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "asyncobjectslot.h"

#include <utility>

namespace KiriView {
quint64 AsyncObjectSlot::start(QObject *object, CancelCallback cancelCallback)
{
    cancel();
    m_object = object;
    m_cancelCallback = std::move(cancelCallback);
    ++m_token;
    return m_token;
}

bool AsyncObjectSlot::accepts(quint64 token, const QObject *object) const
{
    return token == m_token && object == m_object;
}

bool AsyncObjectSlot::claim(quint64 token, QObject *object)
{
    if (!accepts(token, object)) {
        return false;
    }

    m_object = nullptr;
    m_cancelCallback = {};
    return true;
}

void AsyncObjectSlot::clear(QObject *object)
{
    if (object != m_object) {
        return;
    }

    m_object = nullptr;
    m_cancelCallback = {};
}

void AsyncObjectSlot::cancel()
{
    if (m_object == nullptr) {
        return;
    }

    QObject *object = m_object;
    CancelCallback cancelCallback = std::move(m_cancelCallback);
    m_object = nullptr;
    ++m_token;
    if (cancelCallback) {
        cancelCallback(object);
    }
}
}
