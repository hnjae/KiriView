// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imageasyncoperationstate.h"

namespace KiriView {
ImageAsyncOperationState::ImageAsyncOperationState(quint64 nextOperationId)
    : m_nextOperationId(nextOperationId)
{
}

quint64 ImageAsyncOperationState::start()
{
    m_activeOperationId = nextOperationId();
    return m_activeOperationId;
}

bool ImageAsyncOperationState::accepts(quint64 operationId) const
{
    return operationId != 0 && operationId == m_activeOperationId;
}

bool ImageAsyncOperationState::finish(quint64 operationId)
{
    if (!accepts(operationId)) {
        return false;
    }

    m_activeOperationId = 0;
    return true;
}

void ImageAsyncOperationState::cancel() { m_activeOperationId = 0; }

quint64 ImageAsyncOperationState::nextOperationId()
{
    ++m_nextOperationId;
    if (m_nextOperationId == 0) {
        ++m_nextOperationId;
    }
    return m_nextOperationId;
}
}
