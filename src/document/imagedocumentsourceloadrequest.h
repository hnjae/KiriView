// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTSOURCELOADREQUEST_H
#define KIRIVIEW_IMAGEDOCUMENTSOURCELOADREQUEST_H

#include "navigation/imagenavigationtypes.h"

#include <QUrl>

namespace KiriView {
struct ImageDocumentSourceLoadRequest {
    QUrl sourceUrl;
    ImageNavigationCandidateKind sourceKind = ImageNavigationCandidateKind::Image;
    QUrl containerNavigationUrl;
    bool preserveTwoPageSpreadTransition = false;

    static ImageDocumentSourceLoadRequest fromUrl(const QUrl &sourceUrl)
    {
        return fromTarget(ImageNavigationTarget { sourceUrl, ImageNavigationCandidateKind::Image });
    }

    static ImageDocumentSourceLoadRequest fromTarget(const ImageNavigationTarget &target)
    {
        return ImageDocumentSourceLoadRequest { target.url, target.kind, QUrl(), false };
    }

    static ImageDocumentSourceLoadRequest fromContainerImage(
        const QUrl &imageUrl, const QUrl &containerUrl)
    {
        return fromContainerTarget(
            ImageNavigationTarget { imageUrl, ImageNavigationCandidateKind::Image }, containerUrl);
    }

    static ImageDocumentSourceLoadRequest fromContainerTarget(
        const ImageNavigationTarget &target, const QUrl &containerUrl)
    {
        return ImageDocumentSourceLoadRequest { target.url, target.kind, containerUrl, false };
    }

    static ImageDocumentSourceLoadRequest fromPageNavigation(
        const QUrl &sourceUrl, bool preserveTwoPageSpreadTransition)
    {
        return fromPageNavigationTarget(
            ImageNavigationTarget { sourceUrl, ImageNavigationCandidateKind::Image },
            preserveTwoPageSpreadTransition);
    }

    static ImageDocumentSourceLoadRequest fromPageNavigationTarget(
        const ImageNavigationTarget &target, bool preserveTwoPageSpreadTransition)
    {
        return ImageDocumentSourceLoadRequest {
            target.url,
            target.kind,
            QUrl(),
            preserveTwoPageSpreadTransition,
        };
    }
};
}

#endif
