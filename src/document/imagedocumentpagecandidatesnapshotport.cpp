// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentpagecandidatesnapshotport.h"

#include "navigation/imagedocumentpagenavigationservice.h"

namespace kiriview {
ImageDocumentPageCandidateSnapshotPort::ImageDocumentPageCandidateSnapshotPort(
    const ImageDocumentPageNavigationService* navigationService)
    : m_navigationService(navigationService)
{
}

std::optional<ImageDocumentPageCandidateSnapshot>
ImageDocumentPageCandidateSnapshotPort::snapshot() const
{
    if (m_navigationService == nullptr) {
        return std::nullopt;
    }
    return m_navigationService->pageCandidateSnapshot();
}
}
