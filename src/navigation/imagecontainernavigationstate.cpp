// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagecontainernavigationstate.h"

namespace kiriview {
ImageContainerNavigationState::ImageContainerNavigationState(quint64 nextOperationId)
    : m_operation(nextOperationId)
{
}

quint64 ImageContainerNavigationState::startNavigation() { return m_operation.start(); }

bool ImageContainerNavigationState::acceptsNavigation(quint64 operationId) const
{
    return m_operation.accepts(operationId);
}

bool ImageContainerNavigationState::finishNavigation(quint64 operationId)
{
    return m_operation.finish(operationId);
}

void ImageContainerNavigationState::cancel() { m_operation.cancel(); }
}
