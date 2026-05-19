// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEASYNCOPERATIONSTATE_H
#define KIRIVIEW_IMAGEASYNCOPERATIONSTATE_H

#include "imageasyncticket.h"

#include <QtGlobal>

namespace KiriView {
class ImageAsyncOperationState final
{
public:
    explicit ImageAsyncOperationState(quint64 nextOperationId = 0);

    quint64 start();
    bool accepts(quint64 operationId) const;
    bool finish(quint64 operationId);
    void cancel();

private:
    ImageAsyncTicket m_ticket;
    quint64 m_activeOperationId = 0;
};
}

#endif
