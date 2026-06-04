// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATIONCONTROLLER_H

#include "imagedocumentruntimeplan.h"
#include "navigation/imagedocumentpagenavigationtypes.h"

#include <functional>

namespace KiriView {
class ImageDocumentState;
class ImageDocumentPageNavigationService;
class ImagePageSurfaceController;
class ImageSpreadPresentationController;

class ImageDocumentNavigationController final
{
public:
    using RuntimePlanCallback = std::function<void(ImageDocumentRuntimePlan)>;

    ImageDocumentNavigationController(ImageDocumentState &state,
        ImagePageSurfaceController &pageSurfaceController,
        ImageDocumentPageNavigationService &navigationService,
        ImageSpreadPresentationController &spreadController,
        RuntimePlanCallback runtimePlanCallback);

    int currentPageNumber() const;
    int pageCount() const;
    ImageDocumentPageNavigationSnapshot pageNavigationSnapshot() const;

    void openAdjacentPage(NavigationDirection direction);
    void openAdjacentContainer(NavigationDirection direction);
    void openImageAtPage(int pageNumber);
    void openImageAtRelativePageOffset(int offset);

    void updatePageNavigation();
    void cancelNavigation();
    void cancelContainerNavigation();
    void cancelPageNavigationUpdate();
    void cancelAllNavigation();
    void clearPageNavigation();

private:
    ImageDocumentState &m_state;
    ImagePageSurfaceController &m_pageSurfaceController;
    ImageDocumentPageNavigationService &m_navigationService;
    ImageSpreadPresentationController &m_spreadController;
    RuntimePlanCallback m_runtimePlanCallback;
};
}

#endif
