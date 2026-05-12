// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTNAVIGATOR_H
#define KIRIVIEW_IMAGEDOCUMENTNAVIGATOR_H

#include "imagenavigationtypes.h"

namespace KiriView {
class ImageDocumentLoadController;
class ImageDocumentNavigationController;
class ImageSpreadPresentationController;

class ImageDocumentNavigator final
{
public:
    ImageDocumentNavigator(ImageDocumentNavigationController &navigationController,
        ImageSpreadPresentationController &spreadController,
        ImageDocumentLoadController &loadController);

    void openPreviousImage();
    void openNextImage();
    void openPreviousSinglePage();
    void openNextSinglePage();
    void openImageAtPage(int pageNumber);
    void openPreviousContainer();
    void openNextContainer();

private:
    void openAdjacentImage(NavigationDirection direction);
    void openAdjacentContainer(NavigationDirection direction);
    void openImageAtRelativePageOffset(int offset);

    ImageDocumentNavigationController &m_navigationController;
    ImageSpreadPresentationController &m_spreadController;
    ImageDocumentLoadController &m_loadController;
};
}

#endif
