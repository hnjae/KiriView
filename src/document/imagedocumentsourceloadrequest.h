// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADREQUEST_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADREQUEST_H

#include "navigation/imagedocumentpagenavigationtypes.h"

#include <QUrl>

namespace kiriview {
struct ImageDocumentSourceLoadRequest
{
    QUrl sourceUrl;
    ImageDocumentPageKind sourceKind = ImageDocumentPageKind::Image;
    QUrl containerNavigationUrl;
    bool preserveTwoPageSpreadTransition = false;
    bool sameScopeImageNavigation = false;

    static ImageDocumentSourceLoadRequest fromUrl(const QUrl& sourceUrl)
    {
        return fromTarget(ImageDocumentPageTarget { sourceUrl, ImageDocumentPageKind::Image });
    }

    static ImageDocumentSourceLoadRequest fromTarget(const ImageDocumentPageTarget& target)
    {
        return ImageDocumentSourceLoadRequest { target.url, target.kind, QUrl(), false, false };
    }

    static ImageDocumentSourceLoadRequest fromContainerImage(
        const QUrl& imageUrl, const QUrl& containerUrl)
    {
        return fromContainerTarget(
            ImageDocumentPageTarget { imageUrl, ImageDocumentPageKind::Image }, containerUrl);
    }

    static ImageDocumentSourceLoadRequest fromContainerTarget(
        const ImageDocumentPageTarget& target, const QUrl& containerUrl)
    {
        return ImageDocumentSourceLoadRequest { target.url, target.kind, containerUrl, false,
            false };
    }

    static ImageDocumentSourceLoadRequest fromPageNavigation(
        const QUrl& sourceUrl, bool preserveTwoPageSpreadTransition)
    {
        return fromPageNavigationTarget(
            ImageDocumentPageTarget { sourceUrl, ImageDocumentPageKind::Image },
            preserveTwoPageSpreadTransition);
    }

    static ImageDocumentSourceLoadRequest fromPageNavigationTarget(
        const ImageDocumentPageTarget& target, bool preserveTwoPageSpreadTransition)
    {
        return ImageDocumentSourceLoadRequest {
            target.url,
            target.kind,
            QUrl(),
            preserveTwoPageSpreadTransition,
            true,
        };
    }

    static ImageDocumentSourceLoadRequest fromSameScopeImageNavigationUrl(const QUrl& sourceUrl)
    {
        return fromPageNavigation(sourceUrl, true);
    }
};
}

#endif
