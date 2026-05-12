// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H

#include "imagedocumenteffects.h"

namespace KiriView {
class ImageDocumentLoadController;
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageDocumentState;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

class ImageDocumentEffectExecutor final
{
public:
    ImageDocumentEffectExecutor(ImageDocumentState &state,
        ImageDocumentNavigationController &navigationController,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImagePresentationController &presentationController,
        ImageSpreadPresentationController &spreadController,
        ImageDocumentLoadController &loadController);

    void dispatch(ImageDocumentEffect effect);
    void dispatchAll(ImageDocumentEffects effects);

private:
    void dispatchPayload(const ClearImageEffect &);
    void dispatchPayload(const ResetZoomEffect &);
    void dispatchPayload(const UpdatePageNavigationEffect &);
    void dispatchPayload(const ScheduleAdjacentImagePredecodeEffect &);
    void dispatchPayload(const OpenUrlEffect &payload);
    void dispatchPayload(const ContainerImageSelectedEffect &payload);
    void dispatchPayload(const EmptyContainerSelectedEffect &payload);
    void dispatchPayload(const ContainerNavigationFailedEffect &payload);
    void dispatchPayload(const PrepareFailedContainerEffect &payload);

    ImageDocumentState &m_state;
    ImageDocumentNavigationController &m_navigationController;
    ImageDocumentPredecodeController &m_predecodeController;
    ImageOpenController &m_openController;
    ImagePresentationController &m_presentationController;
    ImageSpreadPresentationController &m_spreadController;
    ImageDocumentLoadController &m_loadController;
};
}

#endif
