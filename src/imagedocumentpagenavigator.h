// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATOR_H
#define KIRIVIEW_IMAGEDOCUMENTPAGENAVIGATOR_H

#include "imagenavigationtypes.h"

#include <QUrl>
#include <functional>

namespace KiriView {
class ImageDocumentNavigationController;
class ImageSpreadPresentationController;

class ImageDocumentPageNavigator final
{
public:
    using OpenPageCallback
        = std::function<void(const QUrl &url, bool preserveTwoPageSpreadTransition)>;

    ImageDocumentPageNavigator(ImageDocumentNavigationController &navigationController,
        ImageSpreadPresentationController &spreadController, OpenPageCallback openPage);

    void openPreviousImage();
    void openNextImage();
    void openPreviousSinglePage();
    void openNextSinglePage();
    void openImageAtPage(int pageNumber);

private:
    void openAdjacentImage(NavigationDirection direction);
    void openImageAtRelativePageOffset(int offset);

    ImageDocumentNavigationController &m_navigationController;
    ImageSpreadPresentationController &m_spreadController;
    OpenPageCallback m_openPage;
};
}

#endif
