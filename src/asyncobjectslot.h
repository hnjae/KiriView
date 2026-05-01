// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_ASYNCOBJECTSLOT_H
#define KIRIVIEW_ASYNCOBJECTSLOT_H

#include <QObject>
#include <QtGlobal>
#include <functional>

namespace KiriView {
class AsyncObjectSlot
{
public:
    using CancelCallback = std::function<void(QObject *)>;

    quint64 start(QObject *object, CancelCallback cancelCallback);
    bool accepts(quint64 token, const QObject *object) const;
    bool claim(quint64 token, QObject *object);
    void clear(QObject *object);
    void cancel();

private:
    QObject *m_object = nullptr;
    CancelCallback m_cancelCallback;
    quint64 m_token = 0;
};
}

#endif
