// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H

#include "imagedocumenteffects.h"
#include "imagenavigationtypes.h"

#include <functional>

namespace KiriView {
class ImageDocumentState;
class ImageNavigationService;
class ImagePresentationController;
class ImageSpreadPresentationController;

class ImageDocumentNavigationController final
{
public:
    using EffectCallback = std::function<void(ImageDocumentEffect)>;

    ImageDocumentNavigationController(ImageDocumentState &state,
        ImagePresentationController &presentationController,
        ImageNavigationService &navigationService,
        ImageSpreadPresentationController &spreadController, EffectCallback effectCallback);

    int currentPageNumber() const;
    int imageCount() const;
    ImagePageNavigationSnapshot pageNavigationSnapshot() const;

    void openAdjacentImage(NavigationDirection direction);
    void openAdjacentContainer(NavigationDirection direction);
    void openImageAtPage(int pageNumber);
    void openImageAtRelativePageOffset(int offset);

    void updatePageNavigation();
    void cancelNavigation();
    void cancelContainerNavigation();
    void cancelPageNavigationUpdate();
    void clearPageNavigation();

private:
    ImageDocumentState &m_state;
    ImagePresentationController &m_presentationController;
    ImageNavigationService &m_navigationService;
    ImageSpreadPresentationController &m_spreadController;
    EffectCallback m_effectCallback;
};
}

#endif
