// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONSNAPSHOTPORT_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONSNAPSHOTPORT_H

#include "navigation/imagedocumentpagenavigationtypes.h"

namespace kiriview {
class ImageDocumentPageNavigationService;

class ImageDocumentNavigationSnapshotPort final
{
public:
    explicit ImageDocumentNavigationSnapshotPort(
        const ImageDocumentPageNavigationService* navigationService = nullptr);

    ImageDocumentPageNavigationSnapshot snapshot() const;

private:
    const ImageDocumentPageNavigationService* m_navigationService = nullptr;
};
}

#endif
