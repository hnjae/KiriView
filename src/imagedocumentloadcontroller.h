// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTLOADCONTROLLER_H

#include "imagedocumentsourceloadrequest.h"

namespace KiriView {
class ArchiveDocumentSessionStore;
class ImageDocumentDeletionController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageOpenController;
class ImageSpreadPresentationController;
enum class ImageDocumentRightToLeftReadingTransition;
enum class ImageDocumentSourceLoadUrlTarget;
struct ImageDocumentSourceLoadPlan;

class ImageDocumentLoadController final
{
public:
    ImageDocumentLoadController(ImageDocumentState &state,
        ImageDocumentDeletionController &deletionController,
        ImageDocumentNavigationController &navigationController,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImageSpreadPresentationController &spreadController,
        ArchiveDocumentSessionStore *archiveSessionStore = nullptr);

    void loadSource(const ImageDocumentSourceLoadRequest &request);

private:
    void applySourceLoadPlan(
        const ImageDocumentSourceLoadRequest &request, const ImageDocumentSourceLoadPlan &plan);
    void applyRightToLeftReadingTransition(ImageDocumentRightToLeftReadingTransition transition,
        bool notifyBeforeSourceState, bool notifyAfterOpen);
    void applySourceLoadUrlTarget(
        ImageDocumentSourceLoadUrlTarget target, const ImageDocumentSourceLoadRequest &request);
    void applyContainerNavigationUrlTarget(
        ImageDocumentSourceLoadUrlTarget target, const ImageDocumentSourceLoadRequest &request);
    void applyLoadingContainerNavigationUrlTarget(
        ImageDocumentSourceLoadUrlTarget target, const ImageDocumentSourceLoadRequest &request);

    ImageDocumentState &m_state;
    ImageDocumentDeletionController &m_deletionController;
    ImageDocumentNavigationController &m_navigationController;
    ImageDocumentPredecodeController &m_predecodeController;
    ImageOpenController &m_openController;
    ImageSpreadPresentationController &m_spreadController;
    ArchiveDocumentSessionStore *m_archiveSessionStore = nullptr;
};
}

#endif
