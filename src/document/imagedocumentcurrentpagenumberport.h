// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTCURRENTPAGENUMBERPORT_H
#define KIRIVIEW_IMAGEDOCUMENTCURRENTPAGENUMBERPORT_H

namespace kiriview {
class ImageDocumentPageNavigationService;

class ImageDocumentCurrentPageNumberPort final
{
public:
    explicit ImageDocumentCurrentPageNumberPort(
        const ImageDocumentPageNavigationService *navigationService = nullptr);

    int currentPageNumber() const;

private:
    const ImageDocumentPageNavigationService *m_navigationService = nullptr;
};
}

#endif
