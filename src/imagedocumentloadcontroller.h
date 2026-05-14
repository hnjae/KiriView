// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H

#include "imagedocumenteffects.h"

#include <QUrl>

namespace KiriView {
class ImageDocumentDeletionController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

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

class ImageDocumentLoadController final
{
public:
    ImageDocumentLoadController(ImageDocumentState &state,
        ImageDocumentDeletionController &deletionController,
        ImageDocumentNavigationController &navigationController,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImagePresentationController &presentationController,
        ImageSpreadPresentationController &spreadController);

    void loadSource(const ImageDocumentSourceLoadRequest &request);
    void clearImage();
    ImageDocumentEffects clearAfterSuccessfulFileDeletion();

private:
    ImageDocumentState &m_state;
    ImageDocumentDeletionController &m_deletionController;
    ImageDocumentNavigationController &m_navigationController;
    ImageDocumentPredecodeController &m_predecodeController;
    ImageOpenController &m_openController;
    ImagePresentationController &m_presentationController;
    ImageSpreadPresentationController &m_spreadController;
};
}

#endif
