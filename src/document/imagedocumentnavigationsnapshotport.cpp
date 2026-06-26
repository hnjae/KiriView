// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentnavigationsnapshotport.h"

#include "navigation/imagedocumentpagenavigationservice.h"

namespace kiriview {
ImageDocumentNavigationSnapshotPort::ImageDocumentNavigationSnapshotPort(
    const ImageDocumentPageNavigationService* navigationService)
    : m_navigationService(navigationService)
{
}

ImageDocumentPageNavigationSnapshot ImageDocumentNavigationSnapshotPort::snapshot() const
{
    if (m_navigationService == nullptr) {
        return {};
    }
    return m_navigationService->pageNavigationSnapshot();
}
}
