// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGECONTAINERNAVIGATIONSTATE_H
#define KIRIVIEW_IMAGECONTAINERNAVIGATIONSTATE_H

#include <QtGlobal>

namespace KiriView {
class ImageContainerNavigationState final
{
public:
    explicit ImageContainerNavigationState(quint64 nextOperationId = 0);

    quint64 startNavigation();
    bool acceptsNavigation(quint64 operationId) const;
    bool finishNavigation(quint64 operationId);
    void cancel();

private:
    quint64 nextOperationId();

    quint64 m_nextOperationId = 0;
    quint64 m_activeOperationId = 0;
};
}

#endif
