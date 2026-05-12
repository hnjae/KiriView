// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H

#include "imagedocumenteffects.h"

#include <QUrl>

namespace KiriView {
class ImageDeletionController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageOpenController;
class ImageSpreadPresentationController;

class ImageDocumentLoadController final
{
public:
    ImageDocumentLoadController(ImageDocumentState &state,
        ImageDeletionController &deletionController,
        ImageDocumentNavigationController &navigationController,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImageSpreadPresentationController &spreadController);

    void setSourceUrl(const QUrl &sourceUrl, const QUrl &containerNavigationUrl = QUrl(),
        bool preserveTwoPageSpreadTransition = false);
    ImageDocumentEffects clearAfterSuccessfulFileDeletion();

private:
    void cancelNavigationAndPredecode();

    ImageDocumentState &m_state;
    ImageDeletionController &m_deletionController;
    ImageDocumentNavigationController &m_navigationController;
    ImageDocumentPredecodeController &m_predecodeController;
    ImageOpenController &m_openController;
    ImageSpreadPresentationController &m_spreadController;
};
}

#endif
