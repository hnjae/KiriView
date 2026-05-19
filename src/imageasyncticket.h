// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCTICKET_H
#define KIRIVIEW_IMAGEASYNCTICKET_H

#include <QtGlobal>

namespace KiriView {
class ImageAsyncTicket
{
public:
    explicit ImageAsyncTicket(quint64 current = 0)
        : m_current(current)
    {
    }

    quint64 next()
    {
        advance();
        return m_current;
    }

    void invalidate() { advance(); }

    quint64 current() const { return m_current; }

    bool accepts(quint64 ticket) const { return ticket != 0 && ticket == m_current; }

private:
    void advance()
    {
        ++m_current;
        if (m_current == 0) {
            ++m_current;
        }
    }

    quint64 m_current = 0;
};
}

#endif
