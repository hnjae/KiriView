// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainernavigationstate.h"

namespace KiriView {
ImageContainerNavigationState::ImageContainerNavigationState(quint64 nextOperationId)
    : m_nextOperationId(nextOperationId)
{
}

quint64 ImageContainerNavigationState::startNavigation()
{
    m_activeOperationId = nextOperationId();
    return m_activeOperationId;
}

bool ImageContainerNavigationState::acceptsNavigation(quint64 operationId) const
{
    return operationId != 0 && operationId == m_activeOperationId;
}

bool ImageContainerNavigationState::finishNavigation(quint64 operationId)
{
    if (!acceptsNavigation(operationId)) {
        return false;
    }

    m_activeOperationId = 0;
    return true;
}

void ImageContainerNavigationState::cancel() { m_activeOperationId = 0; }

quint64 ImageContainerNavigationState::nextOperationId()
{
    ++m_nextOperationId;
    if (m_nextOperationId == 0) {
        ++m_nextOperationId;
    }
    return m_nextOperationId;
}
}
