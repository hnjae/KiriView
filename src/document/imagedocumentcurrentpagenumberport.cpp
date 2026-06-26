// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "imagedocumentcurrentpagenumberport.h"

#include "navigation/imagedocumentpagenavigationservice.h"

namespace kiriview {
ImageDocumentCurrentPageNumberPort::ImageDocumentCurrentPageNumberPort(
    const ImageDocumentPageNavigationService* navigationService)
    : m_navigationService(navigationService)
{
}

int ImageDocumentCurrentPageNumberPort::currentPageNumber() const
{
    return m_navigationService != nullptr ? m_navigationService->currentPageNumber() : 0;
}
}
