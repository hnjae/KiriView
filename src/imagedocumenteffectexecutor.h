// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H
#define KIRIVIEW_IMAGEDOCUMENTEFFECTEXECUTOR_H

#include "imagedocumenteffects.h"

#include <QUrl>
#include <functional>

namespace KiriView {
class ImageDocumentNavigationController;
class ImageDocumentPredecodeController;
class ImageOpenController;
class ImagePresentationController;
class ImageSpreadPresentationController;

class ImageDocumentEffectExecutor final
{
public:
    using ClearImageCallback = std::function<void()>;
    using OpenUrlCallback = std::function<void(const QUrl &)>;
    using OpenContainerImageCallback = std::function<void(const QUrl &, const QUrl &)>;

    struct Callbacks {
        ClearImageCallback clearImage;
        OpenUrlCallback openUrl;
        OpenContainerImageCallback openContainerImage;
    };

    ImageDocumentEffectExecutor(ImageDocumentNavigationController &navigationController,
        ImageDocumentPredecodeController &predecodeController, ImageOpenController &openController,
        ImagePresentationController &presentationController,
        ImageSpreadPresentationController &spreadController, Callbacks callbacks);

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

    ImageDocumentNavigationController &m_navigationController;
    ImageDocumentPredecodeController &m_predecodeController;
    ImageOpenController &m_openController;
    ImagePresentationController &m_presentationController;
    ImageSpreadPresentationController &m_spreadController;
    Callbacks m_callbacks;
};
}

#endif
