// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H

#include "imagedocumenteffects.h"
#include "imagedocumentsourceloadrequest.h"

namespace KiriView {
class ImageDocumentDeletionController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;
enum class ImageDocumentSourceLoadAction;
struct ImageDocumentSourceLoadPlan;

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
    void applySourceLoadPlan(
        const ImageDocumentSourceLoadRequest &request, const ImageDocumentSourceLoadPlan &plan);
    void applySourceLoadAction(
        const ImageDocumentSourceLoadRequest &request, ImageDocumentSourceLoadAction action);

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
