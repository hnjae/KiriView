// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCTICKET_H
#define KIRIVIEW_IMAGEASYNCTICKET_H

#include <QtGlobal>

namespace KiriView {
class ImageAsyncTicket
{
public:
    quint64 next()
    {
        ++m_current;
        return m_current;
    }

    void invalidate() { ++m_current; }

    quint64 current() const { return m_current; }

    bool accepts(quint64 ticket) const { return ticket == m_current; }

private:
    quint64 m_current = 0;
};
}

#endif
