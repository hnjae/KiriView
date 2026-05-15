// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADREQUEST_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADREQUEST_H

#include <QUrl>

namespace KiriView {
struct ImageDocumentSourceLoadRequest {
    QUrl sourceUrl;
    QUrl containerNavigationUrl;
    bool preserveTwoPageSpreadTransition = false;

    static ImageDocumentSourceLoadRequest fromUrl(const QUrl &sourceUrl)
    {
        return ImageDocumentSourceLoadRequest { sourceUrl, QUrl(), false };
    }

    static ImageDocumentSourceLoadRequest fromContainerImage(
        const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return ImageDocumentSourceLoadRequest { imageUrl, containerUrl };
    }

    static ImageDocumentSourceLoadRequest fromPageNavigation(
        const QUrl &sourceUrl, bool preserveTwoPageSpreadTransition)
    {
        return ImageDocumentSourceLoadRequest {
            sourceUrl,
            QUrl(),
            preserveTwoPageSpreadTransition,
        };
    }
};
}

#endif
