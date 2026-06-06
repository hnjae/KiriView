// SPDX-FileCopyrightText: 2026 KIM Hyunjae
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef KIRIVIEW_APPLICATIONCOMMANDROUTER_H
#define KIRIVIEW_APPLICATIONCOMMANDROUTER_H

#include "navigation/imageshortcutnavigationpolicy.h"

#include <QtGlobal>
#include <functional>

namespace KiriView::ApplicationActions {
struct ApplicationCommandRouterInput {
    bool imagePannable = false;
    bool rightToLeftReadingActive = false;
    bool videoMode = false;
    bool imageDocumentPageNavigationActive = false;
    bool atKnownFirstActiveNavigation = false;
    bool canOpenPreviousActiveNavigation = false;
};

struct ApplicationCommandRouterPorts {
    std::function<bool()> imageAvailable;
    std::function<bool()> imageViewportHorizontallyPannable;
    std::function<void(double, double)> requestViewportPanBy;
    std::function<bool()> requestViewportScanForward;
    std::function<bool()> requestViewportScanBackward;
    std::function<void()> requestNextDisplayedImageStartToFinalScanPosition;
    std::function<void()> openPreviousSinglePage;
    std::function<void()> openNextSinglePage;
    std::function<void()> requestPreviousActiveNavigationWithBoundary;
    std::function<void()> requestNextActiveNavigationWithBoundary;
    std::function<void()> showFirstImageBoundary;
    std::function<bool()> videoAvailable;
    std::function<bool()> videoSeekable;
    std::function<void(qint64)> seekVideoBy;
};

class ApplicationCommandRouter final
{
public:
    void handleScanForwardAction(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports) const;
    void handleScanBackwardAction(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports) const;
    bool executeHorizontalArrowShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, bool leftArrow) const;
    bool executeSinglePageArrowShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, bool leftArrow) const;
    bool executeVerticalPanShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, bool up) const;
    bool executeVideoSeekShortcut(const ApplicationCommandRouterInput &input,
        const ApplicationCommandRouterPorts &ports, qint64 deltaMilliseconds) const;

private:
    KiriView::ImageShortcutNavigationPolicy m_navigationPolicy;
};
}

#endif
